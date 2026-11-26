#include <Editor/Editor/Panel/Window/InspectorPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Panel/PanelGroup.h>
#include <Editor/Editor/Panel/Window/LayerSettingsPanel.h>
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
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Texture/Image.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Collider/CircleCollider.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>

namespace SeedCore
{
	InspectorPanel::InspectorPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), addComponentPanel_(context, imguiTexture)
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
		ImGuiID dockspaceID = context_.graphics_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		if (ImGui::Begin("インスペクター"))
		{
			if (context_.panel_.tools_->Inspect())
			{
				ImGui::End();
				return;
			}

			Actor actor = lockedActor_ ? lockedActor_ : context_.selection_.Primary();

			if (actor && ActorAccessor::GetEntity(actor).Exists())
			{
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
				addComponentPanel_.Draw(actor);

				if (disabled)
				{
					ImGui::EndDisabled();
				}

				ImGui::EndChild();
			}
			else if (lockedActor_)
			{
				lockedActor_ = Actor();
			}
		}

		ImGui::End();
	}

	void InspectorPanel::DrawName(Actor actor)
	{
		Float iconSize = ImGui::GetTextLineHeight();
		Uint64 lockIcon = lockedActor_ ? imguiTexture_.Icon(String("Misc/Lock")) : imguiTexture_.Icon(String("Misc/Unlock"));

		if (ImGui::ImageButton("##Lock", static_cast<ImTextureID>(lockIcon), ImVec2(iconSize, iconSize)))
		{
			lockedActor_ = lockedActor_ ? Actor() : actor;
		}

		ImGui::SameLine();

		Name* nameComponent = static_cast<Name*>(context_.world_.world_->GetComponent(ActorAccessor::GetEntity(actor), ComponentRegistry::GetComponentID<Name>()));
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
			context_.scene_.history_.Push(MakePtr<ComponentCommand<String>>(*context_.world_.world_, ActorAccessor::GetEntity(actor), ComponentRegistry::GetComponentID<Name>(), 0, oldValue, nameComponent->name_));
		}

		ImGui::SameLine();

		Bool active = actor.Active();
		if (ImGui::Checkbox("有効", &active))
		{
			context_.scene_.history_.Push(MakePtr<ActorActiveCommand>(*context_.world_.world_, actor, active));
			actor.Active(active);
		}
	}

	void InspectorPanel::DrawTags(Actor actor)
	{
		DynamicArray<String> currentTags;
		TagRegistry::ForEach(*WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID()), [&currentTags](const String& tag)
			{
				currentTags.push_back(tag);
			});

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
			context_.scene_.history_.Push(MakePtr<ActorTagCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, removeTag, false));
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
		Bool addClicked = ImGui::ImageButton("##add", static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Plus"))), ImVec2(tagAddIconHeight, tagAddIconHeight));
		ImGui::PopStyleColor();

		if (entered || addClicked)
		{
			std::string text(newTagBuffer_.c_str());
			if (!text.empty())
			{
				String newTag = String(std::string_view(text));
				context_.scene_.history_.Push(MakePtr<ActorTagCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, newTag, true));
				actor.AddTag(newTag);
			}
			std::ranges::fill(newTagBuffer_, '\0');
			ImGui::SetKeyboardFocusHere(-1);
		}

		std::span<const String> allNames = TagRegistry::Names();

		Bool hasActiveTag = std::ranges::any_of(allNames, [](const String& name) { return name != String{}; });

		if (hasActiveTag)
		{
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			for (Size index = 0; index < allNames.size(); ++index)
			{
				if (allNames[index] == String{})
				{
					continue;
				}

				const String& tag = allNames[index];

				ImGui::PushID(static_cast<Int>(index));

				Bool hasTag = actor.HasTag(tag);
				if (ImGui::Checkbox(tag.c_str(), &hasTag))
				{
					context_.scene_.history_.Push(MakePtr<ActorTagCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, tag, hasTag));
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
		std::span<const String> layerNames = LayerRegistry::Names();
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
					context_.scene_.history_.Push(MakePtr<ActorLayerCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, oldLayerName, newLayerName));
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
					LayerRegistry::Name(index, String(std::string_view(layerNameBuffers_[index].c_str())));
					LayerRegistry::Save();
				}

				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Selectable("編集") && context_.panel_.tools_)
			{
				context_.panel_.tools_->Panel<LayerSettingsPanel>().Open();
			}

			ImGui::EndCombo();
		}
	}

	void InspectorPanel::DrawPrefabControls(Actor actor)
	{
		const PrefabRegistry* prefabs = context_.world_.world_->GetService<PrefabRegistry>();
		const PrefabRecord* prefabRecord = prefabs ? prefabs->Record(ActorAccessor::GetEntity(actor).GetID()) : nullptr;
		Uint32 assetID = prefabRecord ? prefabRecord->assetID_ : 0;
		if (assetID == 0)
		{
			return;
		}

		AssetRecord* asset = context_.world_.resource_->Record(assetID);
		if (!asset)
		{
			return;
		}

		ImGui::Text("Prefab: %s", asset->path_.c_str());

		Bool isPlaying = context_.world_.timer_->Playing();
		if (isPlaying)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Prefab に適用"))
		{
			Handle<PrefabBlueprint> handle = context_.world_.resource_->Prefabs().Load(assetID, *context_.world_.resource_);
			PrefabBlueprint* prefab = context_.world_.resource_->Prefabs().Get(handle);
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

	Bool InspectorPanel::DrawComponentEntry(Actor actor, ComponentID componentID, const String& componentName, void* componentData)
	{
		ImGui::PushID(componentData);
		Bool isHeaderOpen = ImGui::TreeNodeEx("##header", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

		Bool removed = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("コンポーネントを削除"))
			{
				context_.scene_.history_.Push(MakePtr<ComponentRemoveCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, componentID, componentName, componentData));
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
		ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(GetComponentIcon(componentName)), ImVec2(contentX, iconY), ImVec2(contentX + iconSize, iconY + iconSize));

		Float textY = headerMin.y + (headerHeight - ImGui::GetTextLineHeight()) * 0.5f;
		ImGui::GetWindowDrawList()->AddText(ImVec2(contentX + iconSize + ImGui::GetStyle().ItemInnerSpacing.x, textY), ImGui::GetColorU32(ImGuiCol_Text), componentName.c_str());
		ImGui::PopID();

		if (removed)
		{
			return true;
		}

		if (isHeaderOpen)
		{
			DrawReflectedFields(componentName, componentData, componentID, ActorAccessor::GetEntity(actor));
		}

		return false;
	}

	void InspectorPanel::DrawComponents(Actor actor)
	{
		Entity entity = ActorAccessor::GetEntity(actor);

		if (const Mesh* mesh = actor.GetComponent<Mesh>(); mesh && mesh->meshID_ != 0)
		{
			Skeleton* skeletonComponent = actor.GetComponent<Skeleton>();
			if (!skeletonComponent || skeletonComponent->skeletonID_ == 0)
			{
				ModelResource* modelResource = context_.graphics_.graphics_->Module().Resources().modelResource_.get();
				Crister* crister = modelResource->Resolve(modelResource->GetHandle(mesh->meshID_));
				AssetRecord* modelAsset = context_.world_.resource_->Record(mesh->meshID_);
				if (crister && modelAsset && !crister->Skins().empty())
				{
					std::string target = (std::filesystem::path(modelAsset->fullpath_.c_str()).parent_path() / (std::filesystem::path(modelAsset->fullpath_.c_str()).stem().string() + ".skeleton")).string();
					std::ranges::replace(target, '\\', '/');
					for (const auto& [assetId, asset] : context_.world_.resource_->Records())
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
			actor.AddComponent<Material>();
		}

		const DynamicArray<ComponentID>& layout = WorldAccessor::Archetypes(*context_.world_.world_).Layout(entity.GetID());

		static const String nameString("Name");
		static const String positionString("Position");
		static const String rotationString("Rotation");
		static const String scaleString("Scale");
		static const String linearVelocityString("LinearVelocity");
		static const String angularVelocityString("AngularVelocity");
		static const String worldMatrixString("WorldMatrix");
		static const String activeString("Active");
		static const String boundsString("Bounds");
		static const String meshString("Mesh");
		static const String materialString("Material");
		static const String skeletonString("Skeleton");
		static const String selectedString("Selected");

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
			ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(imguiTexture_.Icon(String("Component/Transform"))), ImVec2(transformContentX, transformIconY), ImVec2(transformContentX + transformIconSize, transformIconY + transformIconSize));

			Float transformTextY = transformHeaderMin.y + (transformHeaderHeight - ImGui::GetTextLineHeight()) * 0.5f;
			ImGui::GetWindowDrawList()->AddText(ImVec2(transformContentX + transformIconSize + ImGui::GetStyle().ItemInnerSpacing.x, transformTextY), ImGui::GetColorU32(ImGuiCol_Text), "Transform");

			if (transformHeaderOpen)
			{
				Float* positionData = static_cast<Float*>(context_.world_.world_->GetComponent(entity, positionID));
				Rotation* rotation = static_cast<Rotation*>(context_.world_.world_->GetComponent(entity, rotationID));
				Float* scaleData = static_cast<Float*>(context_.world_.world_->GetComponent(entity, scaleID));

				if (positionData)
				{
					DrawTransform(positionData, "位置", linkedPosition_, previousPosition_, entity, positionID);
				}
				if (rotation)
				{
					DrawTransform(rotation, "回転", linkedRotation_, previousRotation_, entity, rotationID);
				}
				if (scaleData)
				{
					DrawTransform(scaleData, "拡大縮小", linkedScale_, previousScale_, entity, scaleID);
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

			void* fixedData = context_.world_.world_->GetComponent(entity, fixedID);
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

			if (
				componentName == nameString ||
				componentName == positionString ||
				componentName == rotationString ||
				componentName == scaleString || 
				componentName == worldMatrixString ||
				componentName == activeString || 
				componentName == boundsString || 
				componentName == meshString
				)
			{
				continue;
			}

			void* componentData = context_.world_.world_->GetComponent(entity, componentID);
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
		for (ComponentID componentID : ComponentRegistry::Registry())
		{
			if (componentID->metadata_->storage_ != ComponentStorage::SparseSet || (componentID->hooks_ && componentID->hooks_->upcast_) || componentID == unknownID)
			{
				continue;
			}

			String componentName = ComponentRegistry::Name(componentID);
			if (
				componentName == linearVelocityString ||
				componentName == angularVelocityString ||
				componentName == selectedString
				)
			{
				continue;
			}

			void* componentData = context_.world_.world_->GetComponent(entity, componentID);
			if (!componentData)
			{
				continue;
			}

			if (DrawComponentEntry(actor, componentID, componentName, componentData))
			{
				break;
			}
		}

		DynamicArray<ComponentID> behaviourIDs;
		for (const SparseSetLocation& location : WorldAccessor::Entities(*context_.world_.world_).Record(entity.GetID())->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviourIDs.push_back(location.id_);
			}
		}
		for (ComponentID componentBaseID : behaviourIDs)
		{
			String componentName = ComponentRegistry::Name(componentBaseID);

			if (
				componentName == materialString || 
				componentName == skeletonString
				)
			{
				continue;
			}

			void* componentData = context_.world_.world_->GetComponent(entity, componentBaseID);
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
					context_.scene_.history_.Push(MakePtr<ComponentRemoveCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, componentBaseID, componentName, componentData));
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
			ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(GetComponentIcon(componentName)), ImVec2(componentBaseContentX, componentBaseIconY), ImVec2(componentBaseContentX + componentBaseIconSize, componentBaseIconY + componentBaseIconSize));

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

	void InspectorPanel::DrawUnknownComponents(Actor actor)
	{
		ComponentID unknownID = ComponentRegistry::GetComponentID<UnknownComponent>();
		if (!unknownID)
		{
			return;
		}
		UnknownComponent* unknown = static_cast<UnknownComponent*>(context_.world_.world_->GetComponent(ActorAccessor::GetEntity(actor), unknownID));
		if (!unknown)
		{
			return;
		}

		for (Size index = 0; index < unknown->components_.size(); ++index)
		{
			ImGui::PushID("UnknownComponent");
			ImGui::PushID(static_cast<Int>(index));
			Bool isHeaderOpen = ImGui::TreeNodeEx("##header", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

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
			ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(GetComponentIcon(String("UnknownComponent"))), ImVec2(contentX, iconY), ImVec2(contentX + iconSize, iconY + iconSize));

			Float textY = headerMin.y + (headerHeight - ImGui::GetTextLineHeight()) * 0.5f;
			ImGui::GetWindowDrawList()->AddText(ImVec2(contentX + iconSize + ImGui::GetStyle().ItemInnerSpacing.x, textY), ImGui::GetColorU32(ImGuiCol_Text), "Unknown Component");
			ImGui::PopID();
			ImGui::PopID();

			if (removed)
			{
				return;
			}

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

		if (componentID->hooks_ && componentID->hooks_->upcast_)
		{
			componentID->hooks_->upcast_(componentData)->DispatchInspectorGUI();
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
			Quaternion quaternion = Transform::Quat(*rotation);
			if (!pendingRotationValid_ || entity.GetID() != pendingRotationEntity_ || quaternion != pendingRotationQuaternion_)
			{
				pendingRotationDegrees_ = Transform::Degree(*rotation);
				pendingRotationEntity_ = entity.GetID();
				pendingRotationQuaternion_ = quaternion;
				pendingRotationValid_ = true;
			}
		}

		Bool isCanvasRotation = false;
		if (isRotation)
		{
			World& world = *context_.world_.world_;
			const Image* image = world.GetComponent<Image>(entity);
			const Text* text = world.GetComponent<Text>(entity);
			const Movie* movie = world.GetComponent<Movie>(entity);
			isCanvasRotation = (image && image->viewType_ == Image::ViewType::Sprite) || (text && text->viewType_ == Text::ViewType::Sprite) || (movie && movie->displayMode_ == Movie::DisplayMode::Sprite) || world.GetComponent<RectCollider>(entity) || world.GetComponent<CircleCollider>(entity);
		}

		Vector3 degree = isRotation ? pendingRotationDegrees_ : Vector3::Zero;
		if (isCanvasRotation)
		{
			std::swap(degree.x, degree.z);
		}
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
				pendingOld_.emplace<Quaternion>(Transform::Quat(*rotation));
			}
			else
			{
				pendingOld_.emplace<Vector3>(Vector3(data[0], data[1], data[2]));
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
			Vector3 editedDegree = degree;
			if (isCanvasRotation)
			{
				std::swap(deltaDegree.x, deltaDegree.z);
				std::swap(editedDegree.x, editedDegree.z);
			}

			Matrix rotationMatrix = Matrix::CreateFromQuaternion(Transform::Quat(*rotation));
			rotationMatrix *= Matrix::CreateFromYawPitchRoll(ToRadians(deltaDegree.y), ToRadians(deltaDegree.x), ToRadians(deltaDegree.z));

			Vector3 discardedScale;
			Vector3 discardedPosition;
			Quaternion quaternion;
			if (rotationMatrix.Decompose(discardedPosition, quaternion, discardedScale))
			{
				Transform::Quat(*rotation, quaternion);
			}

			pendingRotationDegrees_ = editedDegree;
			pendingRotationQuaternion_ = Transform::Quat(*rotation);
		}

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (isRotation)
			{
				Quaternion newQuaternion = Transform::Quat(*rotation);
				if (std::holds_alternative<Quaternion>(pendingOld_) && newQuaternion != std::get<Quaternion>(pendingOld_))
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Quaternion>>(*context_.world_.world_, entity, componentID, 0, std::get<Quaternion>(pendingOld_), newQuaternion));
				}
			}
			else
			{
				Vector3 newValue(data[0], data[1], data[2]);
				if (std::holds_alternative<Vector3>(pendingOld_) && newValue != std::get<Vector3>(pendingOld_))
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Vector3>>(*context_.world_.world_, entity, componentID, 0, std::get<Vector3>(pendingOld_), newValue));
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

				Bool addClicked = false;
				if (field.array_.add_)
				{
					Float iconHeight = ImGui::GetTextLineHeight();
					ImVec2 iconSize(iconHeight, iconHeight);
					Float addButtonWidth = iconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
					ImGui::SameLine(ImGui::GetContentRegionMax().x - addButtonWidth);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					addClicked = ImGui::ImageButton("##add", static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Plus"))), iconSize);
					ImGui::PopStyleColor();
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
							Bool removeClicked = ImGui::ImageButton("##remove", static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Cross"))), iconSize);
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

				if (addClicked)
				{
					field.array_.add_();
				}

				ImGui::PopID();

				index += skipCount;
				continue;
			}

			if (field.array_.size_ > 0 || field.array_.add_)
			{
				Size count = field.array_.size_;
				Bool isPayloadArray = field.assetType_ != PayloadType::None;

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
							context_.scene_.history_.Push(MakePtr<PayloadArrayCommand>(*context_.world_.world_, entity, componentID, field.name_, removeIndex, removedValue, false));
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

					Bool addClicked = false;
					if (field.array_.add_)
					{
						Float iconHeight = ImGui::GetTextLineHeight();
						ImVec2 iconSize(iconHeight, iconHeight);
						Float addButtonWidth = iconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
						ImGui::SameLine(ImGui::GetContentRegionMax().x - addButtonWidth);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						addClicked = ImGui::ImageButton("##add", static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Plus"))), iconSize);
						ImGui::PopStyleColor();
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
								Bool removeClicked = ImGui::ImageButton("##remove", static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Cross"))), iconSize);
								ImGui::PopStyleColor();
								if (removeClicked)
								{
									removeIndex = elementIndex;
								}
								ImGui::SameLine();
								ImGui::PopID();
							}

							Size elementOffset = baseOffset + element.offset_;

							if (element.assetType_ != PayloadType::None)
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

					if (addClicked)
					{
						context_.scene_.history_.Push(MakePtr<ArrayAppendCommand>(*context_.world_.world_, entity, componentID, field.name_, count));
						field.array_.add_();
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

			if (field.assetType_ != PayloadType::None)
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
		Bool changed = false;

		switch (field.type_)
		{
		case AttributeType::Int:
		{
			Int* value = static_cast<Int*>(pointer);
			changed = ImGui::DragInt(label, value, 1.0f, hasClamped ? static_cast<Int>(cMin) : 0, hasClamped ? static_cast<Int>(cMax) : 0);
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<Int>(*value);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<Int>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Int>>(value, std::get<Int>(pendingOld_), *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<Int>(pendingOld_), *value));
				}
			}
			break;
		}
		case AttributeType::Float:
		{
			Float* value = static_cast<Float*>(pointer);
			changed = ImGui::DragFloat(label, value, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<Float>(*value);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<Float>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Float>>(value, std::get<Float>(pendingOld_), *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Float>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<Float>(pendingOld_), *value));
				}
			}
			break;
		}
		case AttributeType::Bool:
		{
			Bool* value = static_cast<Bool*>(pointer);
			Bool oldValue = *value;
			if (changed = ImGui::Checkbox(label, value))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Bool>>(value, oldValue, *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Bool>>(*context_.world_.world_, entity, componentID, fieldOffset, oldValue, *value));
				}
			}
			break;
		}
		case AttributeType::Vector2:
		{
			Vector2* value = static_cast<Vector2*>(pointer);
			changed = ImGui::DragFloat2(label, &value->x, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<Vector2>(*value);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<Vector2>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Vector2>>(value, std::get<Vector2>(pendingOld_), *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Vector2>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<Vector2>(pendingOld_), *value));
				}
			}
			break;
		}
		case AttributeType::Vector3:
		{
			Vector3* value = static_cast<Vector3*>(pointer);
			changed = ImGui::DragFloat3(label, &value->x, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<Vector3>(*value);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<Vector3>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Vector3>>(value, std::get<Vector3>(pendingOld_), *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Vector3>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<Vector3>(pendingOld_), *value));
				}
			}
			break;
		}
		case AttributeType::Vector4:
		{
			Vector4* value = static_cast<Vector4*>(pointer);
			changed = ImGui::DragFloat4(label, &value->x, 0.1f, cMin, cMax);
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<Vector4>(*value);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<Vector4>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Vector4>>(value, std::get<Vector4>(pendingOld_), *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Vector4>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<Vector4>(pendingOld_), *value));
				}
			}
			break;
		}
		case AttributeType::String:
		{
			String* stringValue = static_cast<String*>(pointer);
			std::string textBuffer = stringValue->str();
			textBuffer.resize(1024);
			if (changed = ImGui::InputTextMultiline(label, textBuffer.data(), textBuffer.capacity(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4)))
			{
				*stringValue = String(std::string_view(textBuffer.c_str()));
			}
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<String>(*stringValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<String>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<String>>(stringValue, std::get<String>(pendingOld_), *stringValue));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<String>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<String>(pendingOld_), *stringValue));
				}
			}
			break;
		}
		case AttributeType::Color:
		{
			Color* value = static_cast<Color*>(pointer);
			changed = ImGui::ColorEdit4(label, &value->r);
			if (ImGui::IsItemActivated())
			{
				pendingOld_.emplace<Color>(*value);
			}
			if (ImGui::IsItemDeactivatedAfterEdit() && std::holds_alternative<Color>(pendingOld_))
			{
				if (field.directPtr_)
				{
					context_.scene_.history_.Push(MakePtr<PointerCommand<Color>>(value, std::get<Color>(pendingOld_), *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Color>>(*context_.world_.world_, entity, componentID, fieldOffset, std::get<Color>(pendingOld_), *value));
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
								changed = true;
								if (field.directPtr_)
								{
									context_.scene_.history_.Push(MakePtr<PointerCommand<Int>>(current, oldValue, entry.value_));
								}
								else
								{
									context_.scene_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.world_.world_, entity, componentID, fieldOffset, oldValue, entry.value_));
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

		if (changed && field.revision_)
		{
			void* component = context_.world_.world_->GetComponent(entity, componentID);
			if (component && componentID && componentID->hooks_ && componentID->hooks_->upcast_)
			{
				componentID->hooks_->upcast_(component)->DispatchRevision();
			}
		}
	}

	const Char* InspectorPanel::GetPayloadDropType(PayloadType assetType)const
	{
		switch (assetType)
		{
		case PayloadType::Texture:
			return "ASSET_TEXTURE";
		case PayloadType::Model:
			return "ASSET_MODEL";
		case PayloadType::Effect:
			return "ASSET_EFFECT";
		case PayloadType::Audio:
			return "ASSET_AUDIO";
		case PayloadType::Font:
			return "ASSET_FONT";
		case PayloadType::Movie:
			return "ASSET_MOVIE";
		case PayloadType::Animation:
			return "ASSET_ANIMATION";
		case PayloadType::MeshCollision:
			return "ASSET_MESHCOLLISION";
		case PayloadType::Material:
			return "ASSET_MATERIAL";
		case PayloadType::Skeleton:
			return "ASSET_SKELETON";
		case PayloadType::Sky:
			return "ASSET_SKY";
		case PayloadType::Prefab:
			return "ASSET_PREFAB";
		case PayloadType::Scene:
			return "ASSET_SCENE";
		case PayloadType::Actor:
			return "HIERARCHY_ACTOR";
		default:
			return nullptr;
		}
	}

	Uint64 InspectorPanel::GetComponentIcon(const String& componentName)const
	{
		return imguiTexture_.Icon(String("Component/" + componentName.str()), String("Component/Custom"));
	}

	void InspectorPanel::DrawPayloadField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset)
	{
		Int* value = static_cast<Int*>(pointer);

		const Char* dropType = GetPayloadDropType(field.assetType_);

		if (field.assetType_ == PayloadType::Actor)
		{
			Uint32 targetId = static_cast<Uint32>(*value);
			Actor target = (targetId != 0) ? WorldAccessor::Find(*context_.world_.world_, targetId) : Actor();

			std::string buttonLabel = "ここにドロップ";
			if (target)
			{
				Name* nameComponent = static_cast<Name*>(context_.world_.world_->GetComponent(ActorAccessor::GetEntity(target), ComponentRegistry::GetComponentID<Name>()));
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
					*value = static_cast<Int>(WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(droppedActor).GetID())->persistentID_);
					if (field.directPtr_)
					{
						context_.scene_.history_.Push(MakePtr<PointerCommand<Int>>(value, oldValue, *value));
					}
					else
					{
						context_.scene_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.world_.world_, entity, componentID, fieldOffset, oldValue, *value));
					}
				}
				ImGui::EndDragDropTarget();
			}

			return;
		}

		Uint32 assetId = static_cast<Uint32>(*value);
		AssetRecord* asset = (assetId != 0) ? context_.world_.resource_->Record(assetId) : nullptr;

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
					context_.scene_.history_.Push(MakePtr<PointerCommand<Int>>(value, oldValue, *value));
				}
				else
				{
					context_.scene_.history_.Push(MakePtr<ComponentCommand<Int>>(*context_.world_.world_, entity, componentID, fieldOffset, oldValue, *value));
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void InspectorPanel::DrawPayloadArrayRow(const FieldInfo& field, void* pointer)
	{
		Int* value = static_cast<Int*>(pointer);

		if (field.assetType_ == PayloadType::Actor)
		{
			Uint32 targetId = static_cast<Uint32>(*value);
			Actor target = (targetId != 0) ? WorldAccessor::Find(*context_.world_.world_, targetId) : Actor();

			std::string actorLabel = "(空)";
			if (target)
			{
				Name* nameComponent = static_cast<Name*>(context_.world_.world_->GetComponent(ActorAccessor::GetEntity(target), ComponentRegistry::GetComponentID<Name>()));
				actorLabel = nameComponent ? nameComponent->name_.str() : "(名前なし)";
			}
			ImGui::Selectable(actorLabel.c_str());
			return;
		}

		Uint32 assetId = static_cast<Uint32>(*value);
		AssetRecord* asset = (assetId != 0) ? context_.world_.resource_->Record(assetId) : nullptr;

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
				Int droppedValue = 0;
				if (field.assetType_ == PayloadType::Actor)
				{
					Actor droppedActor = *static_cast<const Actor*>(payload->Data);
					droppedValue = static_cast<Int>(WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(droppedActor).GetID())->persistentID_);
				}
				else
				{
					droppedValue = *static_cast<const Int*>(payload->Data);
				}
				Bool alreadyExists = std::ranges::contains(existingValues, droppedValue);
				if (!alreadyExists)
				{
					context_.scene_.history_.Push(MakePtr<PayloadArrayCommand>(*context_.world_.world_, entity, componentID, field.name_, existingValues.size(), droppedValue, true));
					field.array_.add_();
					*static_cast<Int*>(field.array_.lastPtr_()) = droppedValue;
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
}
