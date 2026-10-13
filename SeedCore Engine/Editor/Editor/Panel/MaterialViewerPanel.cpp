#include <Editor/Editor/Panel/MaterialViewerPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiCommon.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Material/MaterialLoader.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Graphics.h>
#include <GraphicsEngine/Camera/PreviewCamera.h>
#include <GraphicsEngine/Camera/PreviewCameraController.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	MaterialViewerPanel::MaterialViewerPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	MaterialViewerPanel::~MaterialViewerPanel() = default;

	void MaterialViewerPanel::Open()
	{
		show_ = true;
		ImGui::SetWindowFocus("マテリアルビューア");
	}

	Bool MaterialViewerPanel::Visible()const
	{
		return visible_;
	}

	void MaterialViewerPanel::Draw()
	{
		context_.preview_.material_.active_ = false;
		context_.preview_.material_.surfaceAssetID_ = 0;
		visible_ = false;

		if (!show_)
		{
			return;
		}

		ResourceCache* resource = context_.world_.resource_;
		auto slotLabel = [resource](const std::string& slotName, Uint32 assetID, Size slot) -> std::string
		{
			std::string_view slotText = slotName.empty() ? std::string_view("(名前なし)") : std::string_view(slotName);
			if (assetID == 0)
			{
				return std::format("[{}] {} : (モデル内蔵)", slot, slotText);
			}
			AssetRecord* asset = resource->GetAsset(assetID);
			std::string name = asset ? std::filesystem::path(asset->path_.c_str()).stem().string() : std::to_string(assetID);
			return std::format("[{}] {} : {}", slot, slotText, name);
		};

		Actor actor = context_.selection_.Primary();
		Material* material = actor ? context_.world_.world_->GetComponent<Material>(actor.GetEntity()) : nullptr;

		ImGuiWindow* editorViewWindow = ImGui::FindWindowByName("エディタービュー");
		ImGuiID dockID = (editorViewWindow && editorViewWindow->DockId != 0) ? editorViewWindow->DockId : context_.graphics_.imgui_->DockSpaceID();
		ImGui::DockBuilderDockWindow("マテリアルビューア", dockID);
		ImGui::SetNextWindowSize(ImVec2(960, 720), ImGuiCond_FirstUseEver);

		visible_ = ImGui::Begin("マテリアルビューア", &show_);
		if (visible_)
		{
			const Mesh* mesh = actor ? context_.world_.world_->GetComponent<Mesh>(actor.GetEntity()) : nullptr;
			ModelResource* modelResource = context_.graphics_.graphics_->GetGraphicsModule().Resources().modelResource_.get();
			Crister* crister = (mesh && mesh->meshID_ != 0) ? modelResource->Resolve(modelResource->GetHandle(mesh->meshID_)) : nullptr;

			if (!material || !mesh || mesh->meshID_ == 0)
			{
				ImGui::TextDisabled("Material コンポーネントとメッシュを持つアクターを選択してください");
			}
			else if (!crister || crister->Surfaces().empty())
			{
				ImGui::TextDisabled("マテリアルスロットがありません");
			}
			else
			{
				const DynamicArray<Surface>& surfaces = crister->Surfaces();
				if (selected_ >= surfaces.size())
				{
					selected_ = 0;
				}

				material->slotMaterialIDs_.resize(material->slotNames_.size(), 0);

				DynamicArray<Uint32> slotAssetIDs(surfaces.size(), 0);
				for (Size slot = 0; slot < surfaces.size(); slot++)
				{
					if (surfaces[slot].name_.empty())
					{
						continue;
					}
					for (Size index = 0; index < material->slotNames_.size(); index++)
					{
						if (material->slotNames_[index].str() == surfaces[slot].name_)
						{
							Uint32 assignedID = material->slotMaterialIDs_[index];
							slotAssetIDs[slot] = std::ranges::contains(material->materialIDs_, assignedID) ? assignedID : 0;
							break;
						}
					}
				}

				ImGui::SetNextItemWidth(240.0f);
				if (ImGui::BeginCombo("メッシュ", slotLabel(surfaces[selected_].name_, slotAssetIDs[selected_], selected_).c_str()))
				{
					for (Size slot = 0; slot < surfaces.size(); slot++)
					{
						if (ImGui::Selectable(slotLabel(surfaces[slot].name_, slotAssetIDs[slot], slot).c_str(), selected_ == slot))
						{
							selected_ = slot;
						}
					}
					ImGui::EndCombo();
				}

				context_.preview_.material_.active_ = true;
				context_.preview_.material_.meshAssetID_ = mesh->meshID_;
				context_.preview_.material_.surfaceAssetID_ = slotAssetIDs[selected_];

				ImGui::Spacing();

				ImVec2 previewSize = ImGui::GetContentRegionAvail();
				previewSize.y = Max(previewSize.y, 100.0f);

				if (context_.preview_.material_.camera_)
				{
					context_.preview_.material_.camera_->Resize(previewSize.x, previewSize.y);
				}

				ImGui::Image(ImTextureID(context_.graphics_.graphics_->MaterialDisplayGPUHandle().ptr), previewSize);

				Bool orbitHeld = InputSystem::MouseState(InputSystem::MouseButton::Left, InputSystem::IsPressed);
				Bool panHeld = InputSystem::MouseState(InputSystem::MouseButton::Middle, InputSystem::IsPressed);

				if (ImGui::IsItemHovered() && context_.preview_.material_.camera_ && context_.preview_.material_.cameraController_)
				{
					if (orbitHeld || panHeld)
					{
						InputSystem::BeginMouseCapture();
					}
					context_.preview_.material_.cameraController_->Update(*context_.preview_.material_.camera_, ImGui::GetIO().DeltaTime);
				}

				if (!orbitHeld && !panHeld)
				{
					InputSystem::EndMouseCapture();
				}
			}
		}
		ImGui::End();
	}

	void MaterialViewerPanel::DrawDetails()
	{
		Actor actor = context_.selection_.Primary();
		Material* material = actor ? context_.world_.world_->GetComponent<Material>(actor.GetEntity()) : nullptr;
		const Mesh* mesh = actor ? context_.world_.world_->GetComponent<Mesh>(actor.GetEntity()) : nullptr;

		GraphicsModule& graphicsModule = context_.graphics_.graphics_->GetGraphicsModule();
		ResourceCache* cache = context_.world_.resource_;
		ModelResource* modelResource = graphicsModule.Resources().modelResource_.get();
		MaterialResource* materialResource = graphicsModule.Resources().materialResource_.get();
		Crister* crister = (mesh && mesh->meshID_ != 0) ? modelResource->Resolve(modelResource->GetHandle(mesh->meshID_)) : nullptr;

		if (!material || !crister || selected_ >= crister->Surfaces().size())
		{
			ImGui::TextDisabled("マテリアルビューアでメッシュを選択してください");
			return;
		}

		const std::string& slotName = crister->Surfaces()[selected_].name_;
		Uint32 surfaceAssetID = context_.preview_.material_.surfaceAssetID_;

		AssetRecord* assignedAsset = surfaceAssetID != 0 ? cache->GetAsset(surfaceAssetID) : nullptr;
		std::string preview = assignedAsset ? std::filesystem::path(assignedAsset->path_.c_str()).stem().string() : std::string("(内蔵マテリアル)");
		Uint32 newID = surfaceAssetID;

		ImGui::BeginDisabled(slotName.empty());
		if (ImGui::BeginCombo("マテリアル", preview.c_str()))
		{
			if (ImGui::Selectable("(モデル内蔵)", surfaceAssetID == 0))
			{
				newID = 0;
			}
			for (Size paletteIndex = 0; paletteIndex < material->materialIDs_.size(); paletteIndex++)
			{
				Uint32 paletteID = material->materialIDs_[paletteIndex];
				AssetRecord* paletteAsset = paletteID != 0 ? cache->GetAsset(paletteID) : nullptr;
				if (!paletteAsset)
				{
					continue;
				}
				ImGui::PushID(static_cast<Int>(paletteIndex));
				newID = paletteID;
				if (ImGui::Selectable(std::filesystem::path(paletteAsset->path_.c_str()).stem().string().c_str(), paletteID == surfaceAssetID))
				{
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();

		if (newID != surfaceAssetID)
		{
			Size assignmentIndex = material->slotNames_.size();
			for (Size index = 0; index < material->slotNames_.size(); index++)
			{
				if (material->slotNames_[index].str() == slotName)
				{
					assignmentIndex = index;
					break;
				}
			}

			if (newID == 0)
			{
				if (assignmentIndex < material->slotNames_.size())
				{
					material->slotNames_.erase(material->slotNames_.begin() + assignmentIndex);
					material->slotMaterialIDs_.erase(material->slotMaterialIDs_.begin() + assignmentIndex);
				}
			}
			else
			{
				if (assignmentIndex < material->slotNames_.size())
				{
					material->slotMaterialIDs_[assignmentIndex] = newID;
				}
				else
				{
					material->slotNames_.push_back(String(slotName));
					material->slotMaterialIDs_.push_back(newID);
				}
				materialResource->Load(*cache, newID);
			}

			surfaceAssetID = newID;
			context_.preview_.material_.surfaceAssetID_ = newID;
		}

		ImGui::Separator();

		if (surfaceAssetID == 0)
		{
			ImGui::TextDisabled("内蔵マテリアルを使用中");
			return;
		}

		if (editingState_.assetID_ != surfaceAssetID)
		{
			Handle<Surface> handle = materialResource->GetHandle(surfaceAssetID);
			if (handle.empty())
			{
				handle = materialResource->Load(*cache, surfaceAssetID);
			}
			Surface* loaded = graphicsModule.Loaders().materialLoader_->Get(handle);
			if (!loaded)
			{
				ImGui::TextDisabled("マテリアルを読み込めません");
				return;
			}
			editingState_.surface_ = MakePtr<Surface>(*loaded);
			editingState_.assetID_ = surfaceAssetID;
		}

		Surface& surface = *editingState_.surface_;

		if (ImGui::Button("上書き保存"))
		{
			if (AssetRecord* asset = cache->GetAsset(surfaceAssetID))
			{
				graphicsModule.Loaders().materialLoader_->Save(surface, asset->fullpath_);
				materialResource->Unload(surfaceAssetID);
				materialResource->Load(*cache, surfaceAssetID);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("複製して保存"))
		{
			if (AssetRecord* asset = cache->GetAsset(surfaceAssetID))
			{
				std::filesystem::path sourcePath(asset->fullpath_.c_str());
				std::filesystem::path newPath = sourcePath;
				for (Int copyIndex = 2; std::filesystem::exists(newPath); copyIndex++)
				{
					newPath = sourcePath.parent_path() / (sourcePath.stem().string() + " (" + std::to_string(copyIndex) + ")" + sourcePath.extension().string());
				}
				graphicsModule.Loaders().materialLoader_->Save(surface, String(newPath.string()));
				D3D12Context& d3d12Context = context_.graphics_.graphics_->GetContext();
				cache->Reload(d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphics_.graphics_->GetBC7CompressShader());
			}
		}

		ImGui::Separator();

		const Char* shadingModels = "PBR\0Unlit\0Phong\0Toon\0Lambert\0Flat\0Fur\0";
		Int shadingModel = static_cast<Int>(surface.shadingModel_);
		if (ImGui::Combo("シェーディングモデル", &shadingModel, shadingModels))
		{
			surface.shadingModel_ = static_cast<ShadingModel>(shadingModel);
		}

		if (surface.shadingModel_ == ShadingModel::Fur)
		{
			ImGui::SliderFloat("毛の長さ", &surface.furLength_, 0.0f, 0.2f, "%.3f m");
			ImGui::SliderFloat("毛の密度", &surface.furDensity_, 0.0f, 4.0f);
			ImGui::SliderInt("シェル数", &surface.furShellCount_, 1, 32);
		}

		Bool doubleSided = surface.doubleSided_ != 0;
		if (ImGui::Checkbox("両面描画", &doubleSided))
		{
			surface.doubleSided_ = doubleSided ? 1 : 0;
		}

		const Char* alphaModes = "OPAQUE\0MASK\0BLEND\0";
		ImGui::Combo("アルファモード", &surface.alphaMode_, alphaModes);
		if (surface.alphaMode_ == 1)
		{
			ImGui::SliderFloat("アルファカットオフ", &surface.alphaCutoff_, 0.0f, 1.0f);
		}

		ImGui::ColorEdit4("ベースカラー", &surface.baseColor_.x);
		ImGui::SliderFloat("メタリック", &surface.metallic_, 0.0f, 1.0f);
		ImGui::SliderFloat("ラフネス", &surface.roughness_, 0.0f, 1.0f);
		ImGui::ColorEdit3("エミッシブ", surface.emissiveFactor_);

		if (ImGui::CollapsingHeader("KHR_materials"))
		{
			KHR& khr = surface.khr_;

			ImGui::DragFloat("emissive_strength", &khr.emissiveStrength_.emissiveStrength_, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("ior", &khr.ior_.ior_, 0.01f, 1.0f, 3.0f);

			ImGui::SeparatorText("specular");
			ImGui::SliderFloat("specular_factor", &khr.specular_.specularFactor_, 0.0f, 1.0f);
			ImGui::ColorEdit3("specular_color", khr.specular_.specularColorFactor_);

			ImGui::SeparatorText("clearcoat");
			ImGui::SliderFloat("clearcoat_factor", &khr.clearCoat_.clearCoatFactor_, 0.0f, 1.0f);
			ImGui::SliderFloat("clearcoat_roughness", &khr.clearCoat_.clearCoatRoughnessFactor_, 0.0f, 1.0f);

			ImGui::SeparatorText("transmission");
			ImGui::SliderFloat("transmission_factor", &khr.transmission_.transmissionFactor_, 0.0f, 1.0f);

			ImGui::SeparatorText("volume");
			ImGui::DragFloat("thickness_factor", &khr.volume_.thicknessFactor_, 0.001f, 0.0f, 10.0f);
			ImGui::DragFloat("attenuation_distance", &khr.volume_.attenuationDistance_, 0.01f, 0.0f, FLT_MAX);
			ImGui::ColorEdit3("attenuation_color", khr.volume_.attenuationColor_);

			ImGui::SeparatorText("sheen");
			ImGui::ColorEdit3("sheen_color", khr.sheen_.sheenColorFactor_);
			ImGui::SliderFloat("sheen_roughness", &khr.sheen_.sheenRoughnessFactor_, 0.0f, 1.0f);

			ImGui::SeparatorText("iridescence");
			ImGui::SliderFloat("iridescence_factor", &khr.iridescence_.iridescenceFactor_, 0.0f, 1.0f);
			ImGui::DragFloat("iridescence_ior", &khr.iridescence_.iridescenceIor_, 0.01f, 1.0f, 3.0f);
			ImGui::DragFloat("iridescence_thickness_min", &khr.iridescence_.iridescenceThicknessMinimum_, 1.0f, 0.0f, 2000.0f);
			ImGui::DragFloat("iridescence_thickness_max", &khr.iridescence_.iridescenceThicknessMaximum_, 1.0f, 0.0f, 2000.0f);

			ImGui::SeparatorText("anisotropy");
			ImGui::SliderFloat("anisotropy_strength", &khr.anisotropy_.anisotropyStrength_, 0.0f, 1.0f);
			ImGui::SliderAngle("anisotropy_rotation", &khr.anisotropy_.anisotropyRotation_);

			ImGui::SeparatorText("unlit");
			Bool unlit = khr.unlit_.unlit_ != 0;
			if (ImGui::Checkbox("unlit", &unlit))
			{
				khr.unlit_.unlit_ = unlit ? 1 : 0;
			}
		}
	}
}
