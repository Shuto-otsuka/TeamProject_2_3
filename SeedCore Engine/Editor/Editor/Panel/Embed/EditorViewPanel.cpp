#include <Editor/Editor/Panel/Embed/EditorViewPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Interaction/ActorEditing.h>
#include <Editor/Editor/Interaction/ViewportPicking.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Light/PointLight.h>
#include <GraphicsEngine/Light/SpotLight.h>
#include <GraphicsEngine/Light/DirectionalLight.h>
#include <GraphicsEngine/Light/RectangleLight.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Effect/Effect.h>
#include <GraphicsEngine/Texture/Image.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <AudioEngine/Audio/AudioListener.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Graphics/GraphicsDisplay.h>

namespace SeedCore
{
	EditorViewPanel::EditorViewPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), guizmoPanel_(context, imguiTexture)
	{
		/// No Code
	}

	void EditorViewPanel::Draw()
	{
		D3D12_GPU_DESCRIPTOR_HANDLE frameBufferHandle = context_.graphics_.graphics_->Display().View(RenderViewKind::Editor);

		ImVec2 regionSize = ImGui::GetContentRegionAvail();

		if (regionSize.x > 0.0f && regionSize.y > 0.0f)
		{
			constexpr Float aspectRatio = 16.0f / 9.0f;
			Float imageWidth = regionSize.x;
			Float imageHeight = regionSize.x / aspectRatio;

			if (imageHeight > regionSize.y)
			{
				imageHeight = regionSize.y;
				imageWidth = regionSize.y * aspectRatio;
			}

			imageWidth = ImFloor(imageWidth);
			imageHeight = ImFloor(imageHeight);
			Float offsetX = ImFloor((regionSize.x - imageWidth) * 0.5f);
			Float offsetY = ImFloor((regionSize.y - imageHeight) * 0.5f);

			ImVec2 cursorPosition = ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY);
			ImGui::SetCursorPos(cursorPosition);

			ImVec2 screenPosition = ImGui::GetCursorScreenPos();
			screenPosition.x = IM_ROUND(screenPosition.x);
			screenPosition.y = IM_ROUND(screenPosition.y);
			ImGui::SetCursorScreenPos(screenPosition);

			ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 0.0f);
			if (frameBufferHandle.ptr != 0)
			{
				ImGui::Image(ImTextureID(frameBufferHandle.ptr), ImVec2(imageWidth, imageHeight));
			}
			else
			{
				ImGui::Dummy(ImVec2(imageWidth, imageHeight));
			}
			ImGui::PopStyleVar();

			ImVec2 borderMin = ImVec2(screenPosition.x - 1.0f, screenPosition.y - 1.0f);
			ImVec2 borderMax = ImVec2(screenPosition.x + imageWidth + 1.0f, screenPosition.y + imageHeight + 1.0f);
			ImGui::GetWindowDrawList()->AddRect(borderMin, borderMax, ImGui::GetColorU32(ImGuiCol_Border));

			Vector2 imagePosition = Vector2(screenPosition.x, screenPosition.y);
			Vector2 imageSize = Vector2(imageWidth, imageHeight);
			DrawIcon(imagePosition, imageSize);
			UpdatePick(imagePosition, imageSize);
			DrawManipulator(imagePosition, imageSize);
			DrawSpeed(imagePosition);
		}

		UpdateSnap();
		UpdateCamera();

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGuizmo::IsUsing())
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
			{
				ActorEditing::DuplicationSelection(context_);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
			{
				ActorEditing::DeleteSelection(context_);
			}
		}
	}

	void EditorViewPanel::DrawManipulator(const Vector2& position, const Vector2& size)
	{
		if (!context_.view_.editor_.camera_)
		{
			return;
		}

		constexpr Float gizmoSize = 80.0f;
		ImVec2 gizmoPosition = ImVec2(position.x, position.y + size.y - gizmoSize);

		Matrix view = context_.view_.editor_.camera_->View();

		Float orbitDistance = Vector3::Distance(context_.view_.editor_.camera_->Eye(), context_.view_.editor_.camera_->Focus());
		if (orbitDistance < 0.01f)
		{
			orbitDistance = 8.0f;
		}

		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(position.x, position.y, size.x, size.y);
		ImGuizmo::ViewManipulate(reinterpret_cast<Float*>(&view), orbitDistance, gizmoPosition, ImVec2(gizmoSize, gizmoSize), 0x00000000);

		if (ImGuizmo::IsUsingViewManipulate())
		{
			Matrix cameraWorld = view.Invert();

			Vector3 newEye = cameraWorld.Translation();
			Vector3 newForward = Vector3::TransformDirection(Vector3::Forward, cameraWorld);
			newForward.Normalize();

			Vector3 worldUp = Vector3::Up;
			Vector3 newUp = worldUp - newForward * newForward.Dot(worldUp);
			if (newUp.LengthSq() < 1e-6f)
			{
				newUp = context_.view_.editor_.camera_->Up();
			}
			newUp.Normalize();

			context_.view_.editor_.camera_->Eye(newEye);
			context_.view_.editor_.camera_->Focus(newEye + newForward * orbitDistance);
			context_.view_.editor_.camera_->Up(newUp);
		}

		guizmoPanel_.Draw(position, size);
	}

	void EditorViewPanel::DrawIcon(const Vector2& position, const Vector2& size)
	{
		icons_.clear();
		if (!context_.view_.editor_.iconVisible_ || !context_.view_.editor_.camera_ || !context_.world_.world_)
		{
			return;
		}

		Matrix viewProjection = context_.view_.editor_.camera_->NonJitterViewProjection();

		ActorRegistry& actorRegistry = WorldAccessor::Actors(*context_.world_.world_);
		for (EntityID actorID = actorRegistry.Root(); actorID != EntityID{}; actorID = actorRegistry.Next(actorID))
		{
			Actor actor = WorldAccessor::Make(*context_.world_.world_, actorID);
			if (!actor.Active())
			{
				continue;
			}

			ViewIcon icon;
			if (actor.GetComponent<Camera>() || actor.GetComponent<CameraBrain>())
			{
				icon.type_ = String("View/CameraView");
				icon.color_ = Color::FromPacked(IM_COL32(255, 178, 127, 255));
			}
			else if (actor.GetComponent<PointLight>())
			{
				icon.type_ = String("View/PointLightView");
				icon.color_ = Color::FromPacked(IM_COL32(90, 210, 255, 255));
			}
			else if (actor.GetComponent<SpotLight>())
			{
				icon.type_ = String("View/SpotLightView");
				icon.color_ = Color::FromPacked(IM_COL32(90, 210, 255, 255));
			}
			else if (actor.GetComponent<DirectionalLight>())
			{
				icon.type_ = String("View/DirectionalLightView");
				icon.color_ = Color::FromPacked(IM_COL32(90, 210, 255, 255));
			}
			else if (actor.GetComponent<RectangleLight>())
			{
				icon.type_ = String("View/RectangleLightView");
				icon.color_ = Color::FromPacked(IM_COL32(90, 210, 255, 255));
			}
			else if (actor.GetComponent<SkyLight>())
			{
				icon.type_ = String("View/SkyLightView");
				icon.color_ = Color::FromPacked(IM_COL32(255, 210, 150, 255));
			}
			else if (actor.GetComponent<AudioSource>())
			{
				icon.type_ = String("View/AudioSourceView");
				icon.color_ = Color::FromPacked(IM_COL32(180, 224, 111, 255));
			}
			else if (actor.GetComponent<AudioListener>())
			{
				icon.type_ = String("View/AudioListenerView");
				icon.color_ = Color::FromPacked(IM_COL32(180, 224, 111, 255));
			}
			else if (actor.GetComponent<Mesh>() || actor.GetComponent<Effect>() || actor.GetComponent<Image>() || actor.GetComponent<Text>() || actor.GetComponent<Movie>())
			{
				continue;
			}
			else
			{
				icon.type_ = String("View/ActorView");
				icon.color_ = Color::FromPacked(IM_COL32(217, 122, 255, 255));
			}

			Vector3 worldPosition = actor.WorldMatrix().Translation();
			Vector4 clip = Vector4::Transform(Vector4(worldPosition, 1.0f), viewProjection);
			if (clip.w <= 0.0001f)
			{
				continue;
			}

			Float ndcX = clip.x / clip.w;
			Float ndcY = clip.y / clip.w;
			if (ndcX < -1.1f || ndcX > 1.1f || ndcY < -1.1f || ndcY > 1.1f)
			{
				continue;
			}

			icon.actor_ = actor;
			icon.center_ = Vector2(position.x + (ndcX * 0.5f + 0.5f) * size.x, position.y + (0.5f - ndcY * 0.5f) * size.y);
			icon.depth_ = clip.w;
			icons_.push_back(icon);
		}

		std::ranges::sort(icons_, [](const ViewIcon& a, const ViewIcon& b) { return a.depth_ > b.depth_; });

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(ImVec2(position.x, position.y), ImVec2(position.x + size.x, position.y + size.y), true);
		Float half = iconSize_ * 0.5f;
		for (const ViewIcon& icon : icons_)
		{
			drawList->AddCircleFilled(ImVec2(icon.center_.x, icon.center_.y), half + 3.0f, IM_COL32(20, 20, 28, 170));
			drawList->AddImage(imguiTexture_.Icon(icon.type_), ImVec2(icon.center_.x - half, icon.center_.y - half), ImVec2(icon.center_.x + half, icon.center_.y + half), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), icon.color_.ToPacked());
			if (context_.selection_.Contains(icon.actor_))
			{
				drawList->AddCircle(ImVec2(icon.center_.x, icon.center_.y), half + 3.0f, IM_COL32(255, 200, 60, 255), 0, 2.0f);
			}
		}
		drawList->PopClipRect();
	}

	void EditorViewPanel::DrawSpeed(const Vector2& position)
	{
		Bool rotateHeld = InputSystem::MouseState(InputSystem::MouseButton::Right, InputSystem::IsPressed);
		if (!rotateHeld || !ImGui::IsWindowHovered() || !context_.view_.editor_.cameraController_)
		{
			return;
		}

		std::string text = std::format("移動速度: {:.2f}", context_.view_.editor_.cameraController_->MoveSpeed());
		ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
		ImVec2 textPosition = ImVec2(position.x + 8.0f, position.y + 8.0f);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImVec2(textPosition.x - 4.0f, textPosition.y - 2.0f), ImVec2(textPosition.x + textSize.x + 4.0f, textPosition.y + textSize.y + 2.0f), IM_COL32(20, 20, 28, 170), 4.0f);
		drawList->AddText(textPosition, IM_COL32(255, 255, 255, 255), text.c_str());
	}

	void EditorViewPanel::UpdatePick(const Vector2& position, const Vector2& size)
	{
		if (ImGui::IsWindowHovered() && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !guizmoPanel_.RectToolActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && context_.view_.editor_.camera_ && context_.world_.world_)
		{
			ImVec2 mousePosition = ImGui::GetMousePos();
			if (mousePosition.x >= position.x && mousePosition.x <= position.x + size.x && mousePosition.y >= position.y && mousePosition.y <= position.y + size.y)
			{
				EditorCamera& camera = *context_.view_.editor_.camera_;

				Float ndcX = ((mousePosition.x - position.x) / size.x) * 2.0f - 1.0f;
				Float ndcY = 1.0f - ((mousePosition.y - position.y) / size.y) * 2.0f;

				Vector3 rayOrigin = camera.Eye();
				Vector4 farPointH = Vector4::Transform(Vector4(ndcX, ndcY, 1.0f, 1.0f), camera.InverseViewProjection());
				Vector3 farPoint = farPointH.xyz / farPointH.w;
				Vector3 rayDirection = farPoint - rayOrigin;
				rayDirection.Normalize();

				Actor hitActor;
				Float half = iconSize_ * 0.5f;
				for (auto it = icons_.rbegin(); it != icons_.rend(); ++it)
				{
					Float dx = mousePosition.x - it->center_.x;
					Float dy = mousePosition.y - it->center_.y;
					if (dx * dx + dy * dy <= half * half)
					{
						hitActor = it->actor_;
						break;
					}
				}
				if (!hitActor)
				{
					hitActor = ViewportPicking::Pick(*context_.world_.world_, rayOrigin, rayDirection);
				}

				if (ImGui::GetIO().KeyCtrl)
				{
					context_.selection_.Toggle(hitActor);
				}
				else
				{
					context_.selection_.Select(hitActor);
				}
			}
		}
	}

	void EditorViewPanel::UpdateSnap()
	{
		if (ImGui::IsWindowHovered() && InputSystem::KeyState(InputSystem::Key::Control) && context_.guizmo_.operation_ != 0 && !context_.guizmo_.rectTool_)
		{
			auto& guizmo = context_.guizmo_;

			Float wheel = InputSystem::MouseWheel();
			if (Abs(wheel) > 0.0f)
			{
				if (guizmo.operation_ == ImGuizmo::TRANSLATE)
				{
					guizmo.translateSnap_ = Clamp(guizmo.translateSnap_ + wheel * 0.1f, 0.01f, 100.0f);
				}
				else if (guizmo.operation_ == ImGuizmo::ROTATE)
				{
					guizmo.rotateSnap_ = Clamp(guizmo.rotateSnap_ + wheel * 0.5f, 0.1f, 90.0f);
				}
				else if (guizmo.operation_ == ImGuizmo::SCALE)
				{
					guizmo.scaleSnap_ = Clamp(guizmo.scaleSnap_ + wheel * 0.05f, 0.01f, 10.0f);
				}
			}

			if (guizmo.operation_ == ImGuizmo::TRANSLATE)
			{
				ImGui::SetTooltip("移動スナップ: %.2f", guizmo.translateSnap_);
			}
			else if (guizmo.operation_ == ImGuizmo::ROTATE)
			{
				ImGui::SetTooltip("回転スナップ: %.0f\xc2\xb0", guizmo.rotateSnap_);
			}
			else if (guizmo.operation_ == ImGuizmo::SCALE)
			{
				ImGui::SetTooltip("拡大縮小スナップ: %.2f", guizmo.scaleSnap_);
			}
		}
	}

	void EditorViewPanel::UpdateCamera()
	{
		Bool editorRotateHeld = InputSystem::MouseState(InputSystem::MouseButton::Right, InputSystem::IsPressed);
		Bool editorPanHeld = InputSystem::MouseState(InputSystem::MouseButton::Middle, InputSystem::IsPressed);

		if (!ImGuizmo::IsUsing() && ImGui::IsWindowHovered() && context_.view_.editor_.camera_ && context_.view_.editor_.cameraController_)
		{
			if (editorRotateHeld || editorPanHeld)
			{
				InputSystem::BeginMouseCapture();
			}

			Float deltaTime = ImGui::GetIO().DeltaTime;
			context_.view_.editor_.cameraController_->Update(*context_.view_.editor_.camera_, deltaTime);
		}

		if (!editorRotateHeld && !editorPanHeld)
		{
			InputSystem::EndMouseCapture();
		}
	}
}
