#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <Editor/Editor/Panel/Embed/AddComponentPanel.h>

	namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;
	class Actor;

	class InspectorPanel
	{
	public:
		InspectorPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~InspectorPanel() = default;

		void Draw();

	private:
		void DrawName(Actor actor);

		void DrawTags(Actor actor);

		void DrawLayer(Actor actor);

		void DrawPrefabControls(Actor actor);

		void DrawComponents(Actor actor);

		Bool DrawComponentEntry(Actor actor, ComponentID componentID, const String& componentName, void* componentData);

		void DrawUnknownComponents(Actor actor);

		void DrawReflectedFields(String componentName, void* componentData, ComponentID componentID, Entity entity);

		void DrawFieldList(DynamicArray<FieldInfo>& fields, void* baseData, Entity entity, ComponentID componentID, Size baseOffset);

		void DrawField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset);

		void DrawPayloadField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset);

		void DrawPayloadArrayRow(const FieldInfo& field, void* pointer);

		void DrawPayloadArrayAppendSlot(const FieldInfo& field, const DynamicArray<Int>& existingValues, Entity entity, ComponentID componentID);

		void DrawTransform(void* componentData, const Char* label, Bool& linked, Float* previousValues, Entity entity, ComponentID componentID);

		const Char* GetPayloadDropType(PayloadType assetType)const;

		NODISCARD ImTextureID GetComponentIcon(const String& componentName)const;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		AddComponentPanel addComponentPanel_;

		Bool linkedPosition_ = false;
		Bool linkedRotation_ = false;
		Bool linkedScale_ = false;

		Float previousPosition_[3] = {};
		Float previousRotation_[3] = {};
		Float previousScale_[3] = {};

		Actor lockedActor_;

		std::string newTagBuffer_;

		DynamicArray<std::string> layerNameBuffers_;

		std::variant<Int, Float, Vector2, Vector3, Vector4, Quaternion, Color, String> pendingOld_;

		Vector3 pendingRotationDegrees_ = { 0.0f, 0.0f, 0.0f };
		EntityID pendingRotationEntity_;
		Quaternion pendingRotationQuaternion_ = Quaternion::Identity;
		Bool pendingRotationValid_ = false;
	};
}
