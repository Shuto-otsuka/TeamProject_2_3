#include <Editor/Editor/Panel/InspectorPanel.h>
#include <Editor/Editor/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Panel/AnimatorControllerPanel.h>
#include <Editor/Editor/Panel/TimelinePanel.h>
#include <Editor/Editor/Panel/LayerSettingsPanel.h>
#include <Editor/Editor/Panel/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/SkeletonControllerPanel.h>
#include <Editor/Editor/Panel/AvatarPanel.h>
#include <Editor/Editor/Panel/BootScreenPanel.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>
#include <FoundationEngine/World/Command/ComponentCommand.h>
#include <FoundationEngine/World/Command/ComponentLifecycleCommand.h>
#include <FoundationEngine/World/Command/ArrayFieldCommand.h>
#include <FoundationEngine/World/Command/ActorCommand.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <FoundationEngine/World/Tag/TagRegistry.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/LoaderSystem.h>
#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>
#include <GraphicsEngine/Model/Crister.h>

namespace SeedCore
{
	InspectorPanel::InspectorPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), addComponentPanel_(context), imguiTexture_(imguiTexture)
	{
		newTagBuffer_.resize(64);

		layerNameBuffers_.resize(LayerRegistry::LayerCount);
		for (std::string& buffer : layerNameBuffers_)
		{
			buffer.resize(64);
		}
	}

	void InspectorPanel::Draw()
	{
		ImGuiID dockspaceID = context_.graphicsContext_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		if (ImGui::Begin("インスペクター"))
		{
			if (context_.panelContext_.animatorControllerPanel_ && context_.panelContext_.animatorControllerPanel_->Focused())
			{
				context_.panelContext_.animatorControllerPanel_->DrawDetails();
				ImGui::End();
				return;
			}

			if (context_.panelContext_.timelinePanel_ && context_.panelContext_.timelinePanel_->Focused())
			{
				context_.panelContext_.timelinePanel_->DrawDetails();
				ImGui::End();
				return;
			}

			if (context_.panelContext_.materialViewerPanel_ && context_.panelContext_.materialViewerPanel_->Focused())
			{
				context_.panelContext_.materialViewerPanel_->DrawDetails();
				ImGui::End();
				return;
			}

			if (context_.panelContext_.skeletonControllerPanel_ && context_.panelContext_.skeletonControllerPanel_->Focused())
			{
				context_.panelContext_.skeletonControllerPanel_->DrawDetails();
				ImGui::End();
				return;
			}

			if (context_.panelContext_.avatarPanel_ && context_.panelContext_.avatarPanel_->Focused())
			{
				context_.panelContext_.avatarPanel_->DrawDetails();
				ImGui::End();
				return;
			}

			if (context_.panelContext_.bootScreenPanel_ && context_.panelContext_.bootScreenPanel_->Focused())
			{
				context_.panelContext_.bootScreenPanel_->DrawDetails();
				ImGui::End();
				return;
			}

			if (locked_ && !lockedActor_)
			{
				locked_ = false;
				lockedActor_ = Actor();
			}

			Actor actor = locked_ ? lockedActor_ : context_.selectionContext_.selectedActor_;

			if (actor && actor.GetEntity().Exists())
			{
				Bool sharingEditable = !context_.resourceSync_ || ResourceSyncControlPanel::EditableActor(context_, actor, ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseDown(ImGuiMouseButton_Left));
				if (!sharingEditable)
				{
					ImGui::TextDisabled("Shared entity: acquiring edit lease / read only");
				}
				ImGui::BeginDisabled(!sharingEditable);
				ImGui::BeginChild("##InspectorContent", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
				DrawName(actor);

				Float tagLayerColumnWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

				ImGui::BeginChild("##TagColumn", ImVec2(tagLayerColumnWidth, 0.0f), ImGuiChildFlags_AutoResizeY);
				DrawTags(actor);
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("##LayerColumn", ImVec2(tagLayerColumnWidth, 0.0f), ImGuiChildFlags_AutoResizeY);
				DrawLayer(actor);
				ImGui::EndChild();

				DrawPrefabControls(actor);
				ImGui::Separator();

				Bool disabled = !actor.Active();
				if (disabled)
				{
					ImGui::BeginDisabled();
				}

				DrawComponents(actor);
				ImGui::Separator();
				addComponentPanel_.Draw(actor, imguiTexture_);

				if (disabled)
				{
					ImGui::EndDisabled();
				}

				ImGui::EndChild();
				ImGui::EndDisabled();
			}
			else if (locked_)
			{
				locked_ = false;
				lockedActor_ = Actor();
			}
		}

		ImGui::End();
	}

	void InspectorPanel::DrawName(Actor actor)
	{
		Float iconSize = ImGui::GetTextLineHeight();
		ImTextureID lockIcon = locked_ ? imguiTexture_.Icon(IconType::Lock) : imguiTexture_.Icon(IconType::Unlock);

		if (ImGui::ImageButton("##Lock", lockIcon, ImVec2(iconSize, iconSize)))
		{
			locked_ = !locked_;
			lockedActor_ = locked_ ? actor : Actor();
		}

		ImGui::SameLine();

		Name* nameComponent = static_cast<Name*>(context_.worldContext_.world_->GetComponent(actor.GetEntity(), ComponentRegistry::GetComponentID<Name>()));
		if (!nameComponent)
		{
			return;
		}

		std::string nameBuffer = nameComponent->name_.str();
		nameBuffer.resize(256);

		if (ImGui::InputText("名前", nameBuffer.data(), nameBuffer.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			String oldValue = nameComponent->name_;
			nameComponent->name_ = String(std::string_view(nameBuffer.c_str()));
			context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<String>>(*context_.worldContext_.world_, actor.GetEntity(), ComponentRegistry::GetComponentID<Name>(), 0, oldValue, nameComponent->name_));
		}

		ImGui::SameLine();

		Bool active = actor.Active();
		if (ImGui::Checkbox("有効", &active))
		{
			context_.sceneContext_.history_.Push(MakePtr<ActorActiveCommand>(*context_.worldContext_.world_, actor, active));
			actor.Active(active);
		}
	}

	void InspectorPanel::DrawTags(Actor actor)
	{
		DynamicArray<String> currentTags = actor.TagList();

		std::string previewLabel;
		for (Size index = 0; index < currentTags.size(); ++index)
		{
			if (index > 0)
			{
				previewLabel += ", ";
			}
			previewLabel += currentTags[index].str();
		}
		if (previewLabel.empty())
		{
			previewLabel = "(なし)";
		}

		ImGui::TextDisabled("タグ");

		ImGui::SetNextItemWidth(-1.0f);
		if (!ImGui::BeginCombo("##Tags", previewLabel.c_str()))
		{
			return;
		}

		String removeTag;
		Bool hasRemoveTag = false;
		String deleteTag;
		Bool hasDeleteTag = false;

		if (!currentTags.empty())
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			Float windowVisibleX = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

			for (Size index = 0; index < currentTags.size(); ++index)
			{
				const String& tag = currentTags[index];

				ImGui::PushID(tag.c_str());
				ImGui::SmallButton(tag.c_str());

				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("このActorから外す"))
					{
						removeTag = tag;
						hasRemoveTag = true;
					}
					if (ImGui::MenuItem("タグを削除（すべてのActorから）"))
					{
						deleteTag = tag;
						hasDeleteTag = true;
					}
					ImGui::EndPopup();
				}
				else if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("右クリックでメニューを開く");
				}

				ImGui::PopID();

				Float lastButtonX = ImGui::GetItemRectMax().x;
				if (index + 1 < currentTags.size())
				{
					const String& nextTag = currentTags[index + 1];
					Float nextButtonWidth = ImGui::CalcTextSize(nextTag.c_str()).x + style.FramePadding.x * 2.0f;
					Float nextButtonX = lastButtonX + style.ItemSpacing.x + nextButtonWidth;
					if (nextButtonX < windowVisibleX)
					{
						ImGui::SameLine();
					}
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
		}

		if (hasRemoveTag)
		{
			context_.sceneContext_.history_.Push(MakePtr<ActorTagCommand>(*context_.worldContext_.world_, actor.PersistentID(), removeTag, false));
			actor.RemoveTag(removeTag);
		}
		if (hasDeleteTag)
		{
			TagRegistry::Remove(deleteTag);
		}

		ImGui::SetNextItemWidth(140.0f);
		Bool entered = ImGui::InputText("##NewTag", newTagBuffer_.data(), newTagBuffer_.capacity(), ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::SameLine();
		Float tagAddIconHeight = ImGui::GetTextLineHeight();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		Bool addClicked = ImGui::ImageButton("##add", imguiTexture_.Icon(IconType::Add), ImVec2(tagAddIconHeight, tagAddIconHeight));
		ImGui::PopStyleColor();

		if (entered || addClicked)
		{
			std::string text(newTagBuffer_.c_str());
			if (!text.empty())
			{
				String newTag = String(std::string_view(text));
				context_.sceneContext_.history_.Push(MakePtr<ActorTagCommand>(*context_.worldContext_.world_, actor.PersistentID(), newTag, true));
				actor.AddTag(newTag);
			}
			std::ranges::fill(newTagBuffer_, '\0');
			ImGui::SetKeyboardFocusHere(-1);
		}

		const DynamicArray<String>& allNames = TagRegistry::GetNames();

		Bool hasActiveTag = std::ranges::any_of(std::views::iota(Size{ 0 }, allNames.size()), [](Size index) { return !TagRegistry::Removed(index); });

		if (hasActiveTag)
		{
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			for (Size index = 0; index < allNames.size(); ++index)
			{
				if (TagRegistry::Removed(index))
				{
					continue;
				}

				const String& tag = allNames[index];

				ImGui::PushID(static_cast<Int>(index));

				Bool hasTag = actor.HasTag(tag);
				if (ImGui::Checkbox(tag.c_str(), &hasTag))
				{
					context_.sceneContext_.history_.Push(MakePtr<ActorTagCommand>(*context_.worldContext_.world_, actor.PersistentID(), tag, hasTag));
					if (hasTag)
					{
						actor.AddTag(tag);
					}
					else
					{
						actor.RemoveTag(tag);
					}
				}

				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("タグを削除（すべてのActorから）"))
					{
						actor.RemoveTag(tag);
						TagRegistry::Remove(tag);
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
			}
		}

		ImGui::EndCombo();
	}

	void InspectorPanel::DrawLayer(Actor actor)
	{
		const DynamicArray<String>& layerNames = LayerRegistry::GetNames();
		if (layerNames.empty())
		{
			return;
		}

		Size currentLayer = actor.Layer();
		if (currentLayer >= layerNames.size())
		{
			currentLayer = 0;
		}

		ImGui::TextDisabled("レイヤー");

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##Layer", layerNames[currentLayer].c_str()))
		{
			if (ImGui::IsWindowAppearing())
			{
				for (Size index = 0; index < LayerRegistry::LayerCount; ++index)
				{
					std::ranges::fill(layerNameBuffers_[index], '\0');
					std::string name = layerNames[index].str();
					std::ranges::copy(name, layerNameBuffers_[index].begin());
				}
			}

			for (Size index = 0; index < LayerRegistry::LayerCount; ++index)
			{
				ImGui::PushID(static_cast<Int>(index));

				Bool isSelected = (index == currentLayer);
				if (ImGui::Checkbox("##Select", &isSelected) && isSelected)
				{
					String oldLayerName = actor.LayerName();
					String newLayerName = layerNames[index];
					context_.sceneContext_.history_.Push(MakePtr<ActorLayerCommand>(*context_.worldContext_.world_, actor.PersistentID(), oldLayerName, newLayerName));
					actor.Layer(index);
				}

				ImGui::SameLine();
				ImGui::SetNextItemWidth(140.0f);

				if (index == LayerRegistry::DefaultLayer)
				{
					ImGui::BeginDisabled();
					ImGui::InputText("##Name", layerNameBuffers_[index].data(), layerNameBuffers_[index].capacity());
					ImGui::EndDisabled();
				}
				else if (ImGui::InputText("##Name", layerNameBuffers_[index].data(), layerNameBuffers_[index].capacity()))
				{
					LayerRegistry::SetName(index, String(std::string_view(layerNameBuffers_[index].c_str())));
					LayerRegistry::Save();
				}

				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Selectable("編集") && context_.panelContext_.layerSettingsPanel_)
			{
				context_.panelContext_.layerSettingsPanel_->Open();
			}

			ImGui::EndCombo();
		}
	}

	void InspectorPanel::DrawPrefabControls(Actor actor)
	{
		Uint32 assetID = actor.PrefabID();
		if (assetID == 0)
		{
			return;
		}

		AssetRecord* asset = context_.worldContext_.resource_->GetAsset(assetID);
		if (!asset)
		{
			return;
		}

		ImGui::Text("Prefab: %s", asset->path_.c_str());

		Bool isPlaying = context_.worldContext_.gameTimer_->Playing();
		if (isPlaying)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Prefab に適用"))
		{
			Handle<Prefab> handle = context_.worldContext_.resource_->GetPrefabPool().Load(assetID, *context_.worldContext_.resource_);
			Prefab* prefab = context_.worldContext_.resource_->GetPrefabPool().Get(handle);
			if (prefab)
			{
				prefab->Capture(actor);
				prefab->Write(asset->fullpath_.c_str());
			}
		}

		if (isPlaying)
		{
			ImGui::EndDisabled();
		}
	}

	/**
	* [EN]
	* Draws one component's header (with the delete-component context menu)
	* and, if expanded, its reflected fields. Shared by DrawComponents'
	* archetype-layout loop and its sparse-set loop below, since both need the
	* identical header/popup/fields sequence and differ only in how they
	* discovered componentID/componentData.
	*
	* Returns true when the component was removed this frame, so the caller
	* can stop iterating its own component list immediately rather than
	* continuing to reference now-stale data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1コンポーネントぶんのヘッダー(削除用コンテキストメニュー付き)と、
	* 開いていればそのリフレクションフィールドを描画する。下の
	* DrawComponents のアーキタイプ一覧ループとスパースセットループの両方が
	* 共有する — どちらも componentID/componentData の見つけ方が違うだけで、
	* ヘッダー/ポップアップ/フィールド描画の並びは同一なため。
	*
	* このフレームでコンポーネントが削除された場合は true を返す。呼び出し側は
	* 古くなったデータを参照し続けないよう、自分のコンポーネント一覧の走査を
	* 即座に打ち切ること。
	*/
	Bool InspectorPanel::DrawComponentEntry(Actor actor, ComponentID componentID, const String& componentName, void* componentData)
	{
		ImGui::PushID(componentData);
		Bool isHeaderOpen = ImGui::TreeNodeEx("##header", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

		Bool removed = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("コンポーネントを削除"))
			{
				context_.sceneContext_.history_.Push(MakePtr<ComponentRemoveCommand>(*context_.worldContext_.world_, actor.PersistentID(), componentID, componentName, componentData));
				actor.RemoveComponent(componentID);
				removed = true;
			}
			ImGui::EndPopup();
		}

		ImVec2 headerMin = ImGui::GetItemRectMin();
		Float headerHeight = ImGui::GetItemRectSize().y;
		Float iconSize = ImGui::GetTextLineHeight();
		Float contentX = headerMin.x + ImGui::GetTreeNodeToLabelSpacing();
		Float iconY = headerMin.y + (headerHeight - iconSize) * 0.5f;
		ImGui::GetWindowDrawList()->AddImage(GetComponentIcon(componentName), ImVec2(contentX, iconY), ImVec2(contentX + iconSize, iconY + iconSize));

		Float textY = headerMin.y + (headerHeight - ImGui::GetTextLineHeight()) * 0.5f;
		ImGui::GetWindowDrawList()->AddText(ImVec2(contentX + iconSize + ImGui::GetStyle().ItemInnerSpacing.x, textY), ImGui::GetColorU32(ImGuiCol_Text), componentName.c_str());
		ImGui::PopID();

		if (removed)
		{
			return true;
		}

		if (isHeaderOpen)
		{
			DrawReflectedFields(componentName, componentData, componentID, actor.GetEntity());
		}

		return false;
	}

	void InspectorPanel::DrawComponents(Actor actor)
	{
		Entity entity = actor.GetEntity();

		if (const Mesh* mesh = actor.GetComponent<Mesh>(); mesh && mesh->meshID_ != 0)
		{
			Skeleton* skeletonComponent = actor.GetComponent<Skeleton>();
			if (!skeletonComponent || skeletonComponent->skeletonID_ == 0)
			{
				ModelResource* modelResource = context_.worldContext_.resource_->GetResource<ModelResource>(AssetType::Model);
				Crister* crister = modelResource->Resolve(*context_.worldContext_.loader_, modelResource->GetHandle(mesh->meshID_));
				AssetRecord* modelAsset = context_.worldContext_.resource_->GetAsset(mesh->meshID_);
				if (crister && modelAsset && !crister->Skins().empty())
				{
					std::string target = (std::filesystem::path(modelAsset->fullpath_.c_str()).parent_path() / (std::filesystem::path(modelAsset->fullpath_.c_str()).stem().string() + ".skeleton")).string();
					std::ranges::replace(target, '\\', '/');
					for (const auto& [assetId, asset] : context_.worldContext_.resource_->AssetList())
					{
						if (asset.type_ == AssetType::Skeleton && asset.fullpath_.str() == target)
						{
							if (!skeletonComponent)
							{
								skeletonComponent = actor.AddComponent<Skeleton>();
							}
							if (skeletonComponent)
							{
								skeletonComponent->skeletonID_ = assetId;
							}
							break;
						}
					}
				}
			}
		}

		if (const Mesh* mesh = actor.GetComponent<Mesh>(); mesh && mesh->meshID_ != 0 && !actor.GetComponent<Material>())
		{
			Material* materialComponent = actor.AddComponent<Material>();
			ModelResource* modelResource = context_.worldContext_.resource_->GetResource<ModelResource>(AssetType::Model);
			Crister* crister = modelResource->Resolve(*context_.worldContext_.loader_, modelResource->GetHandle(mesh->meshID_));
			AssetRecord* modelAsset = context_.worldContext_.resource_->GetAsset(mesh->meshID_);
			if (materialComponent && crister && modelAsset)
			{
				std::filesystem::path modelPath(modelAsset->fullpath_.c_str());
				std::filesystem::path directory = modelPath.parent_path() / (modelPath.stem().string() + ".Materials");
				const DynamicArray<Surface>& surfaces = crister->Surfaces();
				materialComponent->materialIDs_.resize(surfaces.size(), 0);
				for (Size slot = 0; slot < surfaces.size(); slot++)
				{
					std::string target = (directory / (surfaces[slot].name_ + ".material")).string();
					std::ranges::replace(target, '\\', '/');
					for (const auto& [assetId, asset] : context_.worldContext_.resource_->AssetList())
					{
						if (asset.type_ == AssetType::Material && asset.fullpath_.str() == target)
						{
							materialComponent->materialIDs_[slot] = assetId;
							break;
						}
					}
				}
			}
		}

		const DynamicArray<ComponentID>& layout = context_.worldContext_.world_->GetLayout(entity);

		static const String nameString("Name");
		static const String positionString("Position");
		static const String rotationString("Rotation");
		static const String scaleString("Scale");
		static const String velocityString("Velocity");
		static const String activeString("Active");
		static const String boundsString("Bounds");
		static const String meshString("Mesh");
		static const String materialString("Material");
		static const String skeletonString("Skeleton");

		ComponentID positionID = ComponentRegistry::GetComponentID(positionString);
		ComponentID rotationID = ComponentRegistry::GetComponentID(rotationString);
		ComponentID scaleID = ComponentRegistry::GetComponentID(scaleString);

		Bool hasTransform = positionID && rotationID && scaleID && actor.HasComponent(positionID) && actor.HasComponent(rotationID) && actor.HasComponent(scaleID);

		if (hasTransform)
		{
			Bool transformHeaderOpen = ImGui::TreeNodeEx("##transformHeader", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

			ImVec2 transformHeaderMin = ImGui::GetItemRectMin();
			Float transformHeaderHeight = ImGui::GetItemRectSize().y;
			Float transformIconSize = ImGui::GetTextLineHeight();
			Float transformContentX = transformHeaderMin.x + ImGui::GetTreeNodeToLabelSpacing();
			Float transformIconY = transformHeaderMin.y + (transformHeaderHeight - transformIconSize) * 0.5f;
			ImGui::GetWindowDrawList()->AddImage(imguiTexture_.Icon(IconType::ComponentTransform), ImVec2(transformContentX, transformIconY), ImVec2(transformContentX + transformIconSize, transformIconY + transformIconSize));

			Float transformTextY = transformHeaderMin.y + (transformHeaderHeight - ImGui::GetTextLineHeight()) * 0.5f;
			ImGui::GetWindowDrawList()->AddText(ImVec2(transformContentX + transformIconSize + ImGui::GetStyle().ItemInnerSpacing.x, transformTextY), ImGui::GetColorU32(ImGuiCol_Text), "Transform");

			if (transformHeaderOpen)
			{
				Float* positionData = static_cast<Float*>(context_.worldContext_.world_->GetComponent(entity, positionID));
				Rotation* rotation = static_cast<Rotation*>(context_.worldContext_.world_->GetComponent(entity, rotationID));
				Float* scaleData = static_cast<Float*>(context_.worldContext_.world_->GetComponent(entity, scaleID));

				if (positionData)
				{
					DrawTransform(positionData, "位置", positionLinked_, previousPosition_, entity, positionID);
				}
				if (rotation)
				{
					DrawTransform(rotation, "回転", rotationLinked_, previousRotation_, entity, rotationID);
				}
				if (scaleData)
				{
					DrawTransform(scaleData, "拡大縮小", scaleLinked_, previousScale_, entity, scaleID);
				}
			}
		}

		static const String geometryStack[] = { meshString, materialString, skeletonString };
		for (const String& fixedName : geometryStack)
		{
			ComponentID fixedID = ComponentRegistry::GetComponentID(fixedName);
			if (!fixedID)
			{
				continue;
			}

			void* fixedData = context_.worldContext_.world_->GetComponent(entity, fixedID);
			if (!fixedData)
			{
				continue;
			}

			if (DrawComponentEntry(actor, fixedID, fixedName, fixedData))
			{
				return;
			}
		}

		for (const ComponentID& componentID : layout)
		{
			String componentName = ComponentRegistry::Name(componentID);

			if (componentName == nameString || componentName == positionString || componentName == rotationString || componentName == scaleString || componentName == velocityString || componentName == activeString || componentName == boundsString || componentName == meshString)
			{
				continue;
			}

			void* componentData = context_.worldContext_.world_->GetComponent(entity, componentID);
			if (!componentData)
			{
				continue;
			}

			if (DrawComponentEntry(actor, componentID, componentName, componentData))
			{
				break;
			}
		}

		ComponentID unknownID = ComponentRegistry::GetComponentID<UnknownComponent>();
		for (const auto& [componentID, metadata] : ComponentRegistry::Registry())
		{
			if (metadata.storage_ != ComponentStorage::SparseSet || metadata.isComponentBehaviour_ || componentID == unknownID)
			{
				continue;
			}

			void* componentData = context_.worldContext_.world_->GetComponent(entity, componentID);
			if (!componentData)
			{
				continue;
			}

			String componentName = ComponentRegistry::Name(componentID);

			if (DrawComponentEntry(actor, componentID, componentName, componentData))
			{
				break;
			}
		}

		for (ComponentID componentBaseID : actor.ComponentIDList())
		{
			String componentName = ComponentRegistry::Name(componentBaseID);

			if (componentName == materialString || componentName == skeletonString)
			{
				continue;
			}

			EntityID entityID = entity.GetID();
			void* componentData = context_.worldContext_.world_->GetComponent(entityID, componentBaseID);
			if (!componentData)
			{
				continue;
			}

			ImGui::PushID(componentData);
			Bool isHeaderOpen = ImGui::TreeNodeEx("##header", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

			Bool removed = false;
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("コンポーネントを削除"))
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentRemoveCommand>(*context_.worldContext_.world_, actor.PersistentID(), componentBaseID, componentName, componentData));
					actor.RemoveComponent(componentBaseID);
					removed = true;
				}
				ImGui::EndPopup();
			}

			ImVec2 componentBaseHeaderMin = ImGui::GetItemRectMin();
			Float componentBaseHeaderHeight = ImGui::GetItemRectSize().y;
			Float componentBaseIconSize = ImGui::GetTextLineHeight();
			Float componentBaseContentX = componentBaseHeaderMin.x + ImGui::GetTreeNodeToLabelSpacing();
			Float componentBaseIconY = componentBaseHeaderMin.y + (componentBaseHeaderHeight - componentBaseIconSize) * 0.5f;
			ImGui::GetWindowDrawList()->AddImage(GetComponentIcon(componentName), ImVec2(componentBaseContentX, componentBaseIconY), ImVec2(componentBaseContentX + componentBaseIconSize, componentBaseIconY + componentBaseIconSize));

			Float componentBaseTextY = componentBaseHeaderMin.y + (componentBaseHeaderHeight - ImGui::GetTextLineHeight()) * 0.5f;
			ImGui::GetWindowDrawList()->AddText(ImVec2(componentBaseContentX + componentBaseIconSize + ImGui::GetStyle().ItemInnerSpacing.x, componentBaseTextY), ImGui::GetColorU32(ImGuiCol_Text), componentName.c_str());
			ImGui::PopID();

			if (removed)
			{
				break;
			}

			if (isHeaderOpen)
			{
				ImGui::PushID(componentData);

				DynamicArray<FieldInfo> fields;

				auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
				auto reflectionIt = reflectionRegistry.find(componentName);
				if (reflectionIt != reflectionRegistry.end())
				{
					reflectionIt->second(componentData, fields);
				}

				auto& payloadRegistry = PayloadRegistry::GetRegistry();
				auto payloadIt = payloadRegistry.find(componentName);
				if (payloadIt != payloadRegistry.end())
				{
					payloadIt->second(componentData, fields);
				}

				std::ranges::stable_sort(fields, [](const FieldInfo& a, const FieldInfo& b) { return a.offset_ < b.offset_; });
				DrawFieldList(fields, componentData, entity, componentBaseID, 0);

				static_cast<ComponentBehaviour*>(componentData)->DispatchInspectorGUI();

				ImGui::PopID();
			}
		}

		DrawUnknownComponents(actor);
	}

	/**
	* [EN]
	* Draws one header per component whose type this build does not know,
	* labelled "Unknown Component". Its real name and fields appear only
	* once its script is registered and it turns back into the real
	* component; until then it can only be removed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この実行ファイルが型を知らないコンポーネントごとに、
	* 「Unknown Component」という名前でヘッダーを1つずつ描く。本来の名前と
	* フィールドが出るのは、そのスクリプトが登録されて本来のコンポーネントへ
	* 戻ってから。それまでは削除だけができる。
	*/
	void InspectorPanel::DrawUnknownComponents(Actor actor)
	{
		ComponentID unknownID = ComponentRegistry::GetComponentID<UnknownComponent>();
		if (!unknownID)
		{
			return;
		}
		UnknownComponent* unknown = static_cast<UnknownComponent*>(context_.worldContext_.world_->GetComponent(actor.GetEntity(), unknownID));
		if (!unknown)
		{
			return;
		}

		for (Size index = 0; index < unknown->components_.size(); ++index)
		{
			/// [EN] Same TreeNodeEx + AllowOverlap overlay technique as DrawComponentEntry, keyed by position since the entries share one component.
			/// [JP] DrawComponentEntry と同じ TreeNodeEx + AllowOverlap の重ね描画。各項目は1つのコンポーネントを共有しているため、位置で識別する。
			ImGui::PushID("UnknownComponent");
			ImGui::PushID(static_cast<Int>(index));
			Bool isHeaderOpen = ImGui::TreeNodeEx("##header", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

			/// [EN] Removing drops only this entry, and the holder goes with the last one, so the actor is left with nothing unknown.
			/// [JP] 削除で外すのはこの項目だけ。最後の1つと一緒に保持役も外し、actor に型の分からないものが残らないようにする。
			Bool removed = false;
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("コンポーネントを削除"))
				{
					unknown->components_.erase(unknown->components_.begin() + index);
					if (unknown->components_.empty())
					{
						actor.RemoveComponent(unknownID);
					}
					removed = true;
				}
				ImGui::EndPopup();
			}

			ImVec2 headerMin = ImGui::GetItemRectMin();
			Float headerHeight = ImGui::GetItemRectSize().y;
			Float iconSize = ImGui::GetTextLineHeight();
			Float contentX = headerMin.x + ImGui::GetTreeNodeToLabelSpacing();
			Float iconY = headerMin.y + (headerHeight - iconSize) * 0.5f;
			ImGui::GetWindowDrawList()->AddImage(GetComponentIcon(String("UnknownComponent")), ImVec2(contentX, iconY), ImVec2(contentX + iconSize, iconY + iconSize));

			Float textY = headerMin.y + (headerHeight - ImGui::GetTextLineHeight()) * 0.5f;
			ImGui::GetWindowDrawList()->AddText(ImVec2(contentX + iconSize + ImGui::GetStyle().ItemInnerSpacing.x, textY), ImGui::GetColorU32(ImGuiCol_Text), "Unknown Component");
			ImGui::PopID();
			ImGui::PopID();

			if (removed)
			{
				return;
			}

			/// [EN] The saved values are kept but not shown, since without the script there is no telling what they mean.
			/// [JP] 保存済みの値は保持するが表示はしない。スクリプトが無ければ、その値が何を意味するのか分からないため。
			if (isHeaderOpen)
			{
				ImGui::TextWrapped("このコンポーネントのスクリプトが見つかりません。git pull してビルドすると、元のコンポーネントに戻ります。");
			}
		}
	}

	void InspectorPanel::DrawReflectedFields(String componentName, void* componentData, ComponentID componentID, Entity entity)
	{
		if (!componentData)
		{
			return;
		}

		ImGui::PushID(componentID);

		DynamicArray<FieldInfo> fields;

		auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
		auto reflectionIt = reflectionRegistry.find(componentName);
		if (reflectionIt != reflectionRegistry.end())
		{
			reflectionIt->second(componentData, fields);
		}

		auto& payloadRegistry = PayloadRegistry::GetRegistry();
		auto payloadIt = payloadRegistry.find(componentName);
		if (payloadIt != payloadRegistry.end())
		{
			payloadIt->second(componentData, fields);
		}

		std::ranges::stable_sort(fields, [](const FieldInfo& a, const FieldInfo& b) { return a.offset_ < b.offset_; });
		DrawFieldList(fields, componentData, entity, componentID, 0);

		if (ComponentRegistry::Get(componentID).isComponentBehaviour_)
		{
			static_cast<ComponentBehaviour*>(componentData)->DispatchInspectorGUI();
		}

		ImGui::PopID();
	}

	void InspectorPanel::DrawTransform(void* componentData, const Char* label, Bool& linked, Float* previousValues, Entity entity, ComponentID componentID)
	{
		static const String rotationString("Rotation");
		Bool isRotation = componentID == ComponentRegistry::GetComponentID(rotationString);
		Rotation* rotation = isRotation ? static_cast<Rotation*>(componentData) : nullptr;
		if (isRotation)
		{
			Quaternion quaternion = rotation->Quat();
			if (!hasPendingRotation_ || entity.GetID() != pendingRotationEntity_ || quaternion != pendingRotationQuaternion_)
			{
				pendingRotationDegrees_ = rotation->Degree();
				pendingRotationEntity_ = entity.GetID();
				pendingRotationQuaternion_ = quaternion;
				hasPendingRotation_ = true;
			}
		}

		Vector3 degree = isRotation ? pendingRotationDegrees_ : Vector3::Zero;
		Float* data = isRotation ? &degree.x : static_cast<Float*>(componentData);

		std::string checkboxID = std::string("##Link_") + label;

		ImGui::Checkbox(checkboxID.c_str(), &linked);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("X, Y, Z を連動");
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(300.0f);
		Bool edited = ImGui::DragFloat3(label, data, 0.1f);

		if (ImGui::IsItemActivated())
		{
			if (isRotation)
			{
				pendingOldQuaternion_ = rotation->Quat();
			}
			else
			{
				pendingOldVector3_ = Vector3(data[0], data[1], data[2]);
			}

			previousValues[0] = data[0];
			previousValues[1] = data[1];
			previousValues[2] = data[2];
		}

		if (edited && linked)
		{
			for (Int axis = 0; axis < 3; ++axis)
			{
				if (data[axis] != previousValues[axis])
				{
					Float value = data[axis];
					data[0] = value;
					data[1] = value;
					data[2] = value;
					break;
				}
			}
		}

		if (isRotation && edited)
		{
			Vector3 deltaDegree = degree - Vector3(previousValues[0], previousValues[1], previousValues[2]);
			Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotation->Quat());
			rotationMatrix *= Matrix::CreateFromYawPitchRoll(ToRadians(deltaDegree.y), ToRadians(deltaDegree.x), ToRadians(deltaDegree.z));

			Vector3 discardedScale;
			Vector3 discardedPosition;
			Quaternion quaternion;
			if (rotationMatrix.Decompose(discardedScale, quaternion, discardedPosition))
			{
				rotation->x_ = quaternion.x;
				rotation->y_ = quaternion.y;
				rotation->z_ = quaternion.z;
				rotation->w_ = quaternion.w;
			}

			pendingRotationDegrees_ = degree;
			pendingRotationQuaternion_ = rotation->Quat();
		}

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (isRotation)
			{
				Quaternion newQuaternion = rotation->Quat();
				if (newQuaternion != pendingOldQuaternion_)
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Quaternion>>(*context_.worldContext_.world_, entity, componentID, 0, pendingOldQuaternion_, newQuaternion));
				}
			}
			else
			{
				Vector3 newValue(data[0], data[1], data[2]);
				if (newValue != pendingOldVector3_)
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Vector3>>(*context_.worldContext_.world_, entity, componentID, 0, pendingOldVector3_, newValue));
				}
			}
		}

		previousValues[0] = data[0];
		previousValues[1] = data[1];
		previousValues[2] = data[2];
	}

	void InspectorPanel::DrawFieldList(DynamicArray<FieldInfo>& fields, void* baseData, Entity entity, ComponentID componentID, Size baseOffset)
	{
		for (Size index = 0; index < fields.size(); ++index)
		{
			auto& field = fields[index];

			if (!field.editorVisible_)
			{
				Size skipCount = field.array_.size_;
				index += skipCount;
				continue;
			}

			if (field.type_ == AttributeType::Struct)
			{
				Size skipCount = field.array_.size_;

				if (skipCount == 0 && !field.array_.add_)
				{
					void* nestedData = field.directPtr_ ? field.directPtr_ : (static_cast<Uint8*>(baseData) + field.offset_);

					DynamicArray<FieldInfo> nestedFields;

					auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
					auto reflectionIt = reflectionRegistry.find(field.nestedTypeName_);
					if (reflectionIt != reflectionRegistry.end())
					{
						reflectionIt->second(nestedData, nestedFields);
					}

					auto& payloadRegistry = PayloadRegistry::GetRegistry();
					auto payloadIt = payloadRegistry.find(field.nestedTypeName_);
					if (payloadIt != payloadRegistry.end())
					{
						payloadIt->second(nestedData, nestedFields);
					}

					if (!nestedFields.empty())
					{
						std::ranges::stable_sort(nestedFields, [](const FieldInfo& a, const FieldInfo& b) { return a.offset_ < b.offset_; });

						Bool nestedEnabled = !field.enableIf_ || field.enableIf_(baseData);
						ImGui::BeginDisabled(!nestedEnabled);

						ImGui::PushID(field.name_.c_str());

						if (ImGui::TreeNodeEx(field.name_.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
						{
							Size nestedBaseOffset = baseOffset + field.offset_;

							ImGui::Indent();
							DrawFieldList(nestedFields, nestedData, entity, componentID, nestedBaseOffset);
							ImGui::Unindent();

							ImGui::TreePop();
						}

						ImGui::PopID();

						ImGui::EndDisabled();
					}

					continue;
				}

				ImGui::PushID(field.name_.c_str());

				ImGui::AlignTextToFramePadding();
				Bool arrayOpened = ImGui::TreeNodeEx("##structArr", ImGuiTreeNodeFlags_DefaultOpen);
				ImGui::SameLine();
				ImGui::Text("%s [%zu]", field.name_.c_str(), skipCount);

				if (field.array_.add_)
				{
					Float iconHeight = ImGui::GetTextLineHeight();
					ImVec2 iconSize(iconHeight, iconHeight);
					Float addButtonWidth = iconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
					ImGui::SameLine(ImGui::GetContentRegionMax().x - addButtonWidth);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					Bool addClicked = ImGui::ImageButton("##add", imguiTexture_.Icon(IconType::Add), iconSize);
					ImGui::PopStyleColor();
					if (addClicked)
					{
						field.array_.add_();
					}
				}

				if (arrayOpened)
				{
					Size removeIndex = SIZE_MAX;

					for (Size elementIndex = 0; elementIndex < skipCount && (index + 1 + elementIndex) < fields.size(); ++elementIndex)
					{
						if (elementIndex > 0)
						{
							ImGui::Separator();
						}

						auto& element = fields[index + 1 + elementIndex];
						void* elementData = element.directPtr_ ? element.directPtr_ : (static_cast<Uint8*>(baseData) + element.offset_);

						ImGui::PushID(static_cast<Int>(elementIndex));

						ImGui::AlignTextToFramePadding();
						Bool elementOpened = ImGui::TreeNodeEx("##structArrElement", ImGuiTreeNodeFlags_DefaultOpen);
						ImGui::SameLine();
						ImGui::Text("%zu", elementIndex + 1);

						if (field.array_.remove_)
						{
							Float iconHeight = ImGui::GetTextLineHeight();
							ImVec2 iconSize(iconHeight, iconHeight);
							Float removeButtonWidth = iconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
							ImGui::SameLine(ImGui::GetContentRegionMax().x - removeButtonWidth);
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
							Bool removeClicked = ImGui::ImageButton("##remove", imguiTexture_.Icon(IconType::Remove), iconSize);
							ImGui::PopStyleColor();
							if (removeClicked)
							{
								removeIndex = elementIndex;
							}
						}

						if (elementOpened)
						{
							DynamicArray<FieldInfo> elementFields;

							auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
							auto reflectionIt = reflectionRegistry.find(field.nestedTypeName_);
							if (reflectionIt != reflectionRegistry.end())
							{
								reflectionIt->second(elementData, elementFields);
							}

							auto& payloadRegistry = PayloadRegistry::GetRegistry();
							auto payloadIt = payloadRegistry.find(field.nestedTypeName_);
							if (payloadIt != payloadRegistry.end())
							{
								payloadIt->second(elementData, elementFields);
							}

							std::ranges::stable_sort(elementFields, [](const FieldInfo& a, const FieldInfo& b) { return a.offset_ < b.offset_; });

							ImGui::Indent();
							DrawFieldList(elementFields, elementData, entity, componentID, baseOffset);
							ImGui::Unindent();

							ImGui::TreePop();
						}

						ImGui::PopID();
					}

					if (removeIndex != SIZE_MAX && field.array_.remove_)
					{
						field.array_.remove_(removeIndex);
					}

					ImGui::TreePop();
				}

				ImGui::PopID();

				index += skipCount;
				continue;
			}

			if (field.array_.size_ > 0 || field.array_.add_)
			{
				Size count = field.array_.size_;
				Bool isPayloadArray = field.assetType_ != PayloadAssetType::None;

				ImGui::PushID(field.name_.c_str());

				if (isPayloadArray)
				{
					ImGui::TextUnformatted(field.name_.c_str());

					DynamicArray<Int> existingValues;
					existingValues.reserve(count);
					for (Size elementIndex = 0; elementIndex < count && (index + 1 + elementIndex) < fields.size(); ++elementIndex)
					{
						auto& element = fields[index + 1 + elementIndex];
						void* ptr = element.directPtr_ ? element.directPtr_ : (static_cast<Uint8*>(baseData) + element.offset_);
						existingValues.push_back(*static_cast<Int*>(ptr));
					}

					DrawPayloadArrayAppendSlot(field, existingValues, entity, componentID);

					ImGui::Spacing();
					ImGui::Text("%s一覧:", field.name_.c_str());

					Float listHeight = ImGui::GetTextLineHeightWithSpacing() * 4.0f;

					if (ImGui::BeginListBox("##list", ImVec2(-FLT_MIN, listHeight)))
					{
						Size removeIndex = SIZE_MAX;

						for (Size elementIndex = 0; elementIndex < count && (index + 1 + elementIndex) < fields.size(); ++elementIndex)
						{
							auto& element = fields[index + 1 + elementIndex];
							void* ptr = element.directPtr_ ? element.directPtr_ : (static_cast<Uint8*>(baseData) + element.offset_);
							ImGui::PushID(static_cast<Int>(elementIndex));
							DrawPayloadArrayRow(element, ptr);
							if (field.array_.remove_ && ImGui::BeginPopupContextItem("##payloadRowContext"))
							{
								if (ImGui::MenuItem("削除"))
								{
									removeIndex = elementIndex;
								}
								ImGui::EndPopup();
							}
							ImGui::PopID();
						}

						ImGui::EndListBox();

						if (removeIndex != SIZE_MAX && field.array_.remove_)
						{
							auto& removedElement = fields[index + 1 + removeIndex];
							void* removedPtr = removedElement.directPtr_ ? removedElement.directPtr_ : (static_cast<Uint8*>(baseData) + removedElement.offset_);
							Int removedValue = *static_cast<Int*>(removedPtr);
							context_.sceneContext_.history_.Push(MakePtr<PayloadArrayCommand>(*context_.worldContext_.world_, entity, componentID, field.name_, removeIndex, removedValue, false));
							field.array_.remove_(removeIndex);
						}
					}

					ImGui::Spacing();
				}
				else
				{
					ImGui::AlignTextToFramePadding();
					Bool opened = ImGui::TreeNodeEx("##arr", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::SameLine();
					ImGui::Text("%s [%zu]", field.name_.c_str(), count);

					if (field.array_.add_)
					{
						Float iconHeight = ImGui::GetTextLineHeight();
						ImVec2 iconSize(iconHeight, iconHeight);
						Float addButtonWidth = iconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
						ImGui::SameLine(ImGui::GetContentRegionMax().x - addButtonWidth);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						Bool addClicked = ImGui::ImageButton("##add", imguiTexture_.Icon(IconType::Add), iconSize);
						ImGui::PopStyleColor();
						if (addClicked)
						{
							context_.sceneContext_.history_.Push(MakePtr<ArrayAppendCommand>(*context_.worldContext_.world_, entity, componentID, field.name_, count));
							field.array_.add_();
						}
					}

					if (opened)
					{
						Size removeIndex = SIZE_MAX;
						for (Size elementIndex = 0; elementIndex < count && (index + 1 + elementIndex) < fields.size(); ++elementIndex)
						{
							if (elementIndex > 0)
							{
								ImGui::Separator();
							}

							auto& element = fields[index + 1 + elementIndex];
							void* ptr = element.directPtr_ ? element.directPtr_ : (static_cast<Uint8*>(baseData) + element.offset_);

							if (field.array_.remove_)
							{
								Float iconHeight = ImGui::GetTextLineHeight();
								ImVec2 iconSize(iconHeight, iconHeight);
								ImGui::PushID(static_cast<Int>(elementIndex));
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
								Bool removeClicked = ImGui::ImageButton("##remove", imguiTexture_.Icon(IconType::Remove), iconSize);
								ImGui::PopStyleColor();
								if (removeClicked)
								{
									removeIndex = elementIndex;
								}
								ImGui::SameLine();
								ImGui::PopID();
							}

							Size elementOffset = baseOffset + element.offset_;

							if (element.assetType_ != PayloadAssetType::None)
							{
								DrawPayloadField(element, ptr, entity, componentID, elementOffset);
							}
							else
							{
								DrawField(element, ptr, entity, componentID, elementOffset);
							}
						}

						if (removeIndex != SIZE_MAX && field.array_.remove_)
						{
							field.array_.remove_(removeIndex);
						}

						ImGui::TreePop();
					}
				}

				ImGui::PopID();
				index += count;
				continue;
			}

			void* ptr = field.directPtr_ ? field.directPtr_ : (static_cast<Uint8*>(baseData) + field.offset_);

			Bool enabled = !field.enableIf_ || field.enableIf_(baseData);
			ImGui::BeginDisabled(!enabled);

			Size fieldOffset = baseOffset + field.offset_;

			if (field.assetType_ != PayloadAssetType::None)
			{
				ImGui::PushID(static_cast<Int>(fieldOffset));
				DrawPayloadField(field, ptr, entity, componentID, fieldOffset);
				ImGui::PopID();
			}
			else
			{
				DrawField(field, ptr, entity, componentID, fieldOffset);
			}

			ImGui::EndDisabled();
		}
	}

	void InspectorPanel::DrawField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset)
	{
		const Char* label = field.name_.c_str();

		Float cMin = field.clampMin_;
		Float cMax = field.clampMax_;
		Bool hasClamped = (cMin != -FLT_MAX || cMax != FLT_MAX);

		switch (field.type_)
		{
		case AttributeType::Int:
		{
			Int* value = static_cast<Int*>(pointer);
			ImGui::DragInt(label, value, 1.0f, hasClamped ? static_cast<Int>(cMin) : 0, hasClamped ? static_cast<Int>(cMax) : 0);
			if (ImGui::IsItemActivated())
			{
				pendingOldInt_ = *value;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Int>>(value, pendingOldInt_, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, pendingOldInt_, *value));
				}
			}
			break;
		}
		case AttributeType::Float:
		{
			Float* value = static_cast<Float*>(pointer);
			ImGui::DragFloat(label, value, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOldFloat_ = *value;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Float>>(value, pendingOldFloat_, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Float>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, pendingOldFloat_, *value));
				}
			}
			break;
		}
		case AttributeType::Bool:
		{
			Bool* value = static_cast<Bool*>(pointer);
			Bool oldValue = *value;
			if (ImGui::Checkbox(label, value))
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Bool>>(value, oldValue, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Bool>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, oldValue, *value));
				}
			}
			break;
		}
		case AttributeType::Vector2:
		{
			Vector2* value = static_cast<Vector2*>(pointer);
			ImGui::DragFloat2(label, &value->x, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOldVector2_ = *value;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Vector2>>(value, pendingOldVector2_, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Vector2>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, pendingOldVector2_, *value));
				}
			}
			break;
		}
		case AttributeType::Vector3:
		{
			Vector3* value = static_cast<Vector3*>(pointer);
			ImGui::DragFloat3(label, &value->x, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOldVector3_ = *value;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Vector3>>(value, pendingOldVector3_, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Vector3>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, pendingOldVector3_, *value));
				}
			}
			break;
		}
		case AttributeType::String:
		{
			String* stringValue = static_cast<String*>(pointer);
			std::string textBuffer = stringValue->str();
			textBuffer.resize(1024);
			if (ImGui::InputTextMultiline(label, textBuffer.data(), textBuffer.capacity(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4)))
			{
				*stringValue = String(std::string_view(textBuffer.c_str()));
			}
			if (ImGui::IsItemActivated())
			{
				pendingOldString_ = *stringValue;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<String>>(stringValue, pendingOldString_, *stringValue));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<String>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, pendingOldString_, *stringValue));
				}
			}
			break;
		}
		case AttributeType::Color:
		{
			Color* value = static_cast<Color*>(pointer);
			ImGui::ColorEdit4(label, &value->x);
			if (ImGui::IsItemActivated())
			{
				pendingOldColor_ = *value;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Color>>(value, pendingOldColor_, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Color>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, pendingOldColor_, *value));
				}
			}
			break;
		}
		case AttributeType::Enum:
		{
			Int* current = static_cast<Int*>(pointer);
			const auto* entries = EnumRegistry::GetEntries(field.enum_.typeName_);
			if (entries)
			{
				const Char* preview = "Unknown";
				for (const EnumEntry& entry : *entries)
				{
					if (entry.value_ == *current)
					{
						preview = entry.name_.c_str();
						break;
					}
				}
				if (ImGui::BeginCombo(label, preview))
				{
					for (const EnumEntry& entry : *entries)
					{
						Bool selected = (entry.value_ == *current);
						if (ImGui::Selectable(entry.name_.c_str(), selected))
						{
							Int oldValue = *current;
							*current = entry.value_;
							if (oldValue != entry.value_)
							{
								if (field.directPtr_)
								{
									context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Int>>(current, oldValue, entry.value_));
								}
								else
								{
									context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, oldValue, entry.value_));
								}
							}
						}
						if (selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				ImGui::TextDisabled("%s (enum未登録)", label);
			}
			break;
		}
		default:
			ImGui::TextDisabled("%s (リフレクション未対応の型です)", label);
			break;
		}
	}

	const Char* InspectorPanel::GetPayloadDropType(PayloadAssetType assetType)const
	{
		switch (assetType)
		{
		case PayloadAssetType::Texture:
			return "ASSET_TEXTURE";
		case PayloadAssetType::Model:
			return "ASSET_MODEL";
		case PayloadAssetType::Effect:
			return "ASSET_EFFECT";
		case PayloadAssetType::Audio:
			return "ASSET_AUDIO";
		case PayloadAssetType::Font:
			return "ASSET_FONT";
		case PayloadAssetType::Movie:
			return "ASSET_MOVIE";
		case PayloadAssetType::Animation:
			return "ASSET_ANIMATION";
		case PayloadAssetType::MeshCollision:
			return "ASSET_MESHCOLLISION";
		case PayloadAssetType::Material:
			return "ASSET_MATERIAL";
		case PayloadAssetType::Skeleton:
			return "ASSET_SKELETON";
		case PayloadAssetType::Sky:
			return "ASSET_SKY";
		case PayloadAssetType::Prefab:
			return "ASSET_PREFAB";
		case PayloadAssetType::Actor:
			return "HIERARCHY_ACTOR";
		default:
			return nullptr;
		}
	}

	ImTextureID InspectorPanel::GetComponentIcon(const String& componentName)const
	{
		return imguiTexture_.Icon(ImGuiTexture::ComponentIconType(componentName));
	}

	void InspectorPanel::DrawPayloadField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset)
	{
		Int* value = static_cast<Int*>(pointer);

		const Char* dropType = GetPayloadDropType(field.assetType_);

		/// [EN] PayloadAssetType::Actor references a live Actor by persistent ID via World::FindActor, not a ResourceCache asset -- resolve/accept it separately from every other payload type.
		/// [JP] PayloadAssetType::Actor は ResourceCache のアセットではなく、World::FindActor 経由で永続IDから生きた Actor を参照する -- 他の全ペイロード型とは別に解決/受け付けを行う。
		if (field.assetType_ == PayloadAssetType::Actor)
		{
			Uint32 targetId = static_cast<Uint32>(*value);
			Actor target = (targetId != 0) ? context_.worldContext_.world_->FindActor(targetId) : Actor();

			std::string buttonLabel = "ここにドロップ";
			if (target)
			{
				Name* nameComponent = static_cast<Name*>(context_.worldContext_.world_->GetComponent(target.GetEntity(), ComponentRegistry::GetComponentID<Name>()));
				buttonLabel = nameComponent ? nameComponent->name_.str() : "(名前なし)";
			}

			ImGui::Text("%s", field.name_.c_str());
			ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 0));

			if (dropType && ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dropType))
				{
					Actor droppedActor = *static_cast<const Actor*>(payload->Data);
					Int oldValue = *value;
					*value = static_cast<Int>(droppedActor.PersistentID());
					if (field.directPtr_)
					{
						context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Int>>(value, oldValue, *value));
					}
					else
					{
						context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, oldValue, *value));
					}
				}
				ImGui::EndDragDropTarget();
			}

			return;
		}

		Uint32 assetId = static_cast<Uint32>(*value);
		AssetRecord* asset = (assetId != 0) ? context_.worldContext_.resource_->GetAsset(assetId) : nullptr;

		std::string buttonLabel = asset ? std::filesystem::path(asset->path_.c_str()).filename().string() : "ここにドロップ";

		ImGui::Text("%s", field.name_.c_str());
		ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 0));

		if (dropType && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dropType))
			{
				Int oldValue = *value;
				*value = *static_cast<const Int*>(payload->Data);
				if (field.directPtr_)
				{
					context_.sceneContext_.history_.Push(MakePtr<PointerCommand<Int>>(value, oldValue, *value));
				}
				else
				{
					context_.sceneContext_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.worldContext_.world_, entity, componentID, fieldOffset, oldValue, *value));
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void InspectorPanel::DrawPayloadArrayRow(const FieldInfo& field, void* pointer)
	{
		Int* value = static_cast<Int*>(pointer);
		Uint32 assetId = static_cast<Uint32>(*value);
		AssetRecord* asset = (assetId != 0) ? context_.worldContext_.resource_->GetAsset(assetId) : nullptr;

		std::string label = asset ? std::filesystem::path(asset->path_.c_str()).filename().string() : "(空)";
		ImGui::Selectable(label.c_str());
	}

	void InspectorPanel::DrawPayloadArrayAppendSlot(const FieldInfo& field, const DynamicArray<Int>& existingValues, Entity entity, ComponentID componentID)
	{
		const Char* dropType = GetPayloadDropType(field.assetType_);
		if (!dropType || !field.array_.add_ || !field.array_.lastPtr_)
		{
			return;
		}

		ImGui::Button("ここにドロップ", ImVec2(-1, 0));

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dropType))
			{
				Int droppedValue = *static_cast<const Int*>(payload->Data);
				Bool alreadyExists = std::ranges::contains(existingValues, droppedValue);
				if (!alreadyExists)
				{
					context_.sceneContext_.history_.Push(MakePtr<PayloadArrayCommand>(*context_.worldContext_.world_, entity, componentID, field.name_, existingValues.size(), droppedValue, true));
					field.array_.add_();
					*static_cast<Int*>(field.array_.lastPtr_()) = droppedValue;
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
}
