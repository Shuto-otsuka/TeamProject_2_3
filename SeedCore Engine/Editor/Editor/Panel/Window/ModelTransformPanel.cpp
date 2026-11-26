#include <Editor/Editor/Panel/Window/ModelTransformPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiCommon.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Preview/PreviewRequest.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Camera/PreviewCamera.h>
#include <GraphicsEngine/Camera/PreviewCameraController.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <GraphicsEngine/Model/Crister/CristerConverter.h>

namespace SeedCore
{
	namespace
	{
		constexpr Float inspectorColumnWidthRatio = 0.32f;

		const Char* SignedAxisLabel(SignedAxis axis)
		{
			switch (axis)
			{
			case SignedAxis::PositiveX:
				return "+X";
			case SignedAxis::NegativeX:
				return "-X";
			case SignedAxis::PositiveY:
				return "+Y";
			case SignedAxis::NegativeY:
				return "-Y";
			case SignedAxis::PositiveZ:
				return "+Z";
			case SignedAxis::NegativeZ:
			default:
				return "-Z";
			}
		}

		Bool DrawAxisCombo(const Char* label, SignedAxis& axis)
		{
			constexpr SignedAxis options[] = { SignedAxis::PositiveX, SignedAxis::NegativeX, SignedAxis::PositiveY, SignedAxis::NegativeY, SignedAxis::PositiveZ, SignedAxis::NegativeZ };

			Bool changed = false;
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo(label, SignedAxisLabel(axis)))
			{
				for (SignedAxis option : options)
				{
					Bool selected = (axis == option);
					if (ImGui::Selectable(SignedAxisLabel(option), selected))
					{
						axis = option;
						changed = true;
					}
				}
				ImGui::EndCombo();
			}
			return changed;
		}
	}

	ModelTransformPanel::ModelTransformPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void ModelTransformPanel::Open()
	{
		open_ = true;
	}

	void ModelTransformPanel::Open(Uint32 assetId)
	{
		open_ = true;
		SetTarget(assetId);

		const Mesh* mesh = context_.selection_.Primary() ? context_.selection_.Primary().GetComponent<Mesh>() : nullptr;
		lastSelectionMeshID_ = (mesh && mesh->meshID_ != 0) ? mesh->meshID_ : 0;
	}

	void ModelTransformPanel::SetTarget(Uint32 assetId)
	{
		targetMeshAssetID_ = assetId;
		AssetMeta meta;
		AssetRecord* asset = context_.world_.resource_->Record(assetId);
		if (asset)
		{
			meta.Read(std::filesystem::path(asset->fullpath_.c_str()));
		}
		editConvention_ = meta.axisConvention_;
		baseTransformPosition_ = Vector3(0.0f, 0.0f, 0.0f);
		baseTransformRotation_ = Vector3(0.0f, 0.0f, 0.0f);
		baseTransformScale_ = Vector3(1.0f, 1.0f, 1.0f);
		baseTransformPivot_ = Vector3(0.0f, 0.0f, 0.0f);
	}

	void ModelTransformPanel::Draw()
	{
		if (!open_)
		{
			context_.preview_.modelTransform_.active_ = false;
			context_.preview_.modelTransform_.worldMatrix_ = Matrix::Identity;
			return;
		}

		context_.preview_.modelTransform_.active_ = false;
		context_.preview_.modelTransform_.worldMatrix_ = Matrix::Identity;

		ImGuiWindow* editorViewWindow = ImGui::FindWindowByName("エディタービュー");
		ImGuiID dockID = (editorViewWindow && editorViewWindow->DockId != 0) ? editorViewWindow->DockId : context_.graphics_.imgui_->DockSpaceID();
		ImGui::DockBuilderDockWindow("モデル変換", dockID);
		ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("モデル変換", &open_))
		{
			const Mesh* mesh = context_.selection_.Primary() ? context_.selection_.Primary().GetComponent<Mesh>() : nullptr;
			Uint32 selectionMeshId = (mesh && mesh->meshID_ != 0) ? mesh->meshID_ : 0;

			if (selectionMeshId != 0 && selectionMeshId != lastSelectionMeshID_)
			{
				SetTarget(selectionMeshId);
			}
			lastSelectionMeshID_ = selectionMeshId;

			if (targetMeshAssetID_ == 0)
			{
				ImGui::TextDisabled("対象のメッシュがありません");
			}
			else
			{
				ModelResource* modelResource = context_.graphics_.graphics_->Module().Resources().modelResource_.get();
				Handle<Crister> handle = modelResource->GetHandle(targetMeshAssetID_);
				Crister* crister = handle.empty() ? nullptr : modelResource->Resolve(handle);

				if (!crister)
				{
					ImGui::TextDisabled("モデルを読み込み中です...");
				}
				else
				{
					Float inspectorWidth = ImGui::GetContentRegionAvail().x * inspectorColumnWidthRatio;

					ImGui::BeginChild("##previewColumn", ImVec2(-inspectorWidth, 0.0f), true);
					{
						DrawPreview();
					}
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("##axisInspectorColumn", ImVec2(0.0f, 0.0f), true);
					{
						DrawAxisInspector(targetMeshAssetID_);
						DrawBaseTransformInspector(targetMeshAssetID_);
					}
					ImGui::EndChild();
				}
			}
		}
		ImGui::End();
	}

	void ModelTransformPanel::DrawPreview()
	{
		context_.preview_.modelTransform_.active_ = true;
		context_.preview_.modelTransform_.meshAssetID_ = targetMeshAssetID_;

		Matrix baseTransformLinearBasis = Matrix::CreateScale(baseTransformScale_.x, baseTransformScale_.y, baseTransformScale_.z) * Matrix::CreateFromYawPitchRoll(ToRadians(baseTransformRotation_.y), ToRadians(baseTransformRotation_.x), ToRadians(baseTransformRotation_.z));
		context_.preview_.modelTransform_.worldMatrix_ = Matrix::CreateTranslation(-baseTransformPivot_) * baseTransformLinearBasis * Matrix::CreateTranslation(baseTransformPivot_ + baseTransformPosition_);

		ImVec2 previewSize = ImGui::GetContentRegionAvail();
		previewSize.y = Max(previewSize.y - ImGui::GetFrameHeightWithSpacing(), 100.0f);
		context_.preview_.modelTransform_.camera_.Resize(previewSize.x, previewSize.y);

		ImVec2 imagePosition = ImGui::GetCursorScreenPos();
		D3D12_GPU_DESCRIPTOR_HANDLE previewHandle = context_.graphics_.graphics_->PreviewDisplay(PreviewSlot::ModelTransform);
		if (previewHandle.ptr != 0)
		{
			ImGui::Image(ImTextureID(previewHandle.ptr), previewSize);
		}
		else
		{
			ImGui::Dummy(previewSize);
		}

		Bool orbitHeld = InputSystem::MouseState(InputSystem::MouseButton::Left, InputSystem::IsPressed);
		Bool panHeld = InputSystem::MouseState(InputSystem::MouseButton::Middle, InputSystem::IsPressed);

		if (!ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && ImGui::IsItemHovered())
		{
			if (orbitHeld || panHeld)
			{
				InputSystem::BeginMouseCapture();
			}

			context_.preview_.modelTransform_.cameraController_.Update(context_.preview_.modelTransform_.camera_, ImGui::GetIO().DeltaTime);
		}

		if (!orbitHeld && !panHeld)
		{
			InputSystem::EndMouseCapture();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_W))
		{
			baseTransformGizmoOperation_ = ImGuizmo::TRANSLATE;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_E))
		{
			baseTransformGizmoOperation_ = ImGuizmo::ROTATE;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_R))
		{
			baseTransformGizmoOperation_ = ImGuizmo::SCALE;
		}

		if (ImGui::RadioButton("移動(W)", baseTransformGizmoOperation_ == ImGuizmo::TRANSLATE))
		{
			baseTransformGizmoOperation_ = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("回転(E)", baseTransformGizmoOperation_ == ImGuizmo::ROTATE))
		{
			baseTransformGizmoOperation_ = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("拡縮(R)", baseTransformGizmoOperation_ == ImGuizmo::SCALE))
		{
			baseTransformGizmoOperation_ = ImGuizmo::SCALE;
		}

		{
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(imagePosition.x, imagePosition.y, previewSize.x, previewSize.y);
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::AllowAxisFlip(false);

			Matrix view = context_.preview_.modelTransform_.camera_.View();
			Matrix projection = context_.preview_.modelTransform_.camera_.Projection();

			Matrix anchorWorld = Matrix::CreateScale(baseTransformScale_.x, baseTransformScale_.y, baseTransformScale_.z) * Matrix::CreateFromYawPitchRoll(ToRadians(baseTransformRotation_.y), ToRadians(baseTransformRotation_.x), ToRadians(baseTransformRotation_.z)) * Matrix::CreateTranslation(baseTransformPivot_ + baseTransformPosition_);

			if (ImGuizmo::Manipulate(&view._11, &projection._11, baseTransformGizmoOperation_, ImGuizmo::LOCAL, &anchorWorld._11))
			{
				Vector3 newScale, newTranslation;
				Quaternion newRotation;
				if (anchorWorld.Decompose(newTranslation, newRotation, newScale))
				{
					baseTransformScale_ = newScale;

					Vector3 euler = newRotation.ToEuler();
					baseTransformRotation_ = Vector3(ToDegrees(euler.x), ToDegrees(euler.y), ToDegrees(euler.z));

					baseTransformPosition_ = newTranslation - baseTransformPivot_;
				}
			}
		}
	}

	void ModelTransformPanel::DrawAxisInspector(Uint32 assetId)
	{
		ImGui::TextDisabled("軸コンベンション");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Up");
		DrawAxisCombo("##up", editConvention_.up_);

		ImGui::Text("Right");
		DrawAxisCombo("##right", editConvention_.right_);

		ImGui::Text("Forward");
		DrawAxisCombo("##forward", editConvention_.forward_);

		ResolvedAxisConvention resolved = ResolvedAxisConvention::Resolve(editConvention_);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (!resolved.valid_)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "無効な軸の組み合わせです（同じ軸を複数回選択しています）");
		}
		else
		{
			ImGui::Text("利き手 (自動判定): %s", resolved.isRightHanded_ ? "右手系" : "左手系");
		}

		ImGui::Spacing();
		ImGui::Text("巻き順");
		Int windingOverride = static_cast<Int>(editConvention_.windingOverride_);
		if (ImGui::RadioButton("自動", windingOverride == static_cast<Int>(WindingOverride::Auto)))
		{
			editConvention_.windingOverride_ = WindingOverride::Auto;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("そのまま", windingOverride == static_cast<Int>(WindingOverride::AsIs)))
		{
			editConvention_.windingOverride_ = WindingOverride::AsIs;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("反転", windingOverride == static_cast<Int>(WindingOverride::Flip)))
		{
			editConvention_.windingOverride_ = WindingOverride::Flip;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::BeginDisabled(!resolved.valid_);
		if (ImGui::Button("適用"))
		{
			ApplyConversion(assetId);
		}
		ImGui::EndDisabled();
	}

	void ModelTransformPanel::ApplyConversion(Uint32 assetId)
	{
		AssetRecord* asset = context_.world_.resource_->Record(assetId);
		if (!asset)
		{
			return;
		}

		std::filesystem::path path(asset->fullpath_.c_str());
		Bool isSourceAsset = (path.extension() == ".gltf" || path.extension() == ".glb");

		ModelResource* modelResource = context_.graphics_.graphics_->Module().Resources().modelResource_.get();

		AssetMeta meta;
		meta.guid_ = assetId;
		meta.Read(path);
		auto writeConvention = [&meta, &path, this]()
			{
				meta.axisConvention_ = editConvention_;
				meta.Write(path);
			};

		if (isSourceAsset)
		{
			writeConvention();
			modelResource->Unload(assetId);
			modelResource->Load(*context_.world_.resource_, assetId);
		}
		else
		{
			AxisConvention previousConvention = meta.axisConvention_;
			ResolvedAxisConvention oldResolved = ResolvedAxisConvention::Resolve(previousConvention);
			ResolvedAxisConvention newResolved = ResolvedAxisConvention::Resolve(editConvention_);
			Matrix deltaBasis = oldResolved.basis_.Transpose() * newResolved.basis_;

			Handle<Crister> handle = modelResource->GetHandle(assetId);
			Crister* crister = handle.empty() ? nullptr : modelResource->Resolve(handle);
			CristerConverter converter;
			if (!crister || !converter.ApplyAxisConversion(*crister, deltaBasis, newResolved.flipWinding_, path))
			{
				return;
			}

			writeConvention();
			modelResource->Unload(assetId);
			modelResource->Load(*context_.world_.resource_, assetId);
		}
	}

	void ModelTransformPanel::DrawBaseTransformInspector(Uint32 assetId)
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("基礎トランスフォーム");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Position");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::DragFloat3("##basePosition", &baseTransformPosition_.x, 0.01f);

		ImGui::Text("Rotation");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::DragFloat3("##baseRotation", &baseTransformRotation_.x, 0.1f);

		ImGui::Text("Scale");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::DragFloat3("##baseScale", &baseTransformScale_.x, 0.01f);

		ImGui::Text("Pivot");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::DragFloat3("##basePivot", &baseTransformPivot_.x, 0.01f);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		AssetRecord* asset = context_.world_.resource_->Record(assetId);
		std::filesystem::path path = asset ? std::filesystem::path(asset->fullpath_.c_str()) : std::filesystem::path();
		Bool isSourceAsset = (path.extension() == ".gltf" || path.extension() == ".glb");

		if (isSourceAsset)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "軸コンベンションを一度適用して .crister へ変換してから使用してください");
		}

		ImGui::BeginDisabled(isSourceAsset);
		if (ImGui::Button("適用##baseTransform"))
		{
			ApplyTransformConversion(assetId);
		}
		ImGui::EndDisabled();
	}

	void ModelTransformPanel::ApplyTransformConversion(Uint32 assetId)
	{
		AssetRecord* asset = context_.world_.resource_->Record(assetId);
		if (!asset)
		{
			return;
		}

		std::filesystem::path path(asset->fullpath_.c_str());
		if (path.extension() == ".gltf" || path.extension() == ".glb")
		{
			return;
		}

		ModelResource* modelResource = context_.graphics_.graphics_->Module().Resources().modelResource_.get();
		Handle<Crister> handle = modelResource->GetHandle(assetId);
		Crister* crister = handle.empty() ? nullptr : modelResource->Resolve(handle);
		CristerConverter converter;
		if (!crister || !converter.ApplyTransformConversion(*crister, baseTransformPosition_, baseTransformRotation_, baseTransformScale_, baseTransformPivot_, path))
		{
			return;
		}

		Matrix baseTransformLinearBasis = Matrix::CreateScale(baseTransformScale_.x, baseTransformScale_.y, baseTransformScale_.z) * Matrix::CreateFromYawPitchRoll(ToRadians(baseTransformRotation_.y), ToRadians(baseTransformRotation_.x), ToRadians(baseTransformRotation_.z));
		Matrix appliedTransform = Matrix::CreateTranslation(-baseTransformPivot_) * baseTransformLinearBasis * Matrix::CreateTranslation(baseTransformPivot_ + baseTransformPosition_);
		AssetMeta meta;
		meta.guid_ = assetId;
		meta.Read(path);
		meta.modelTransform_ = meta.modelTransform_ * appliedTransform;
		meta.Write(path);

		baseTransformPosition_ = Vector3(0.0f, 0.0f, 0.0f);
		baseTransformRotation_ = Vector3(0.0f, 0.0f, 0.0f);
		baseTransformScale_ = Vector3(1.0f, 1.0f, 1.0f);
		baseTransformPivot_ = Vector3(0.0f, 0.0f, 0.0f);

		modelResource->Unload(assetId);
		modelResource->Load(*context_.world_.resource_, assetId);
	}
}
