#pragma once
#include <FoundationEngine/Reflection/ReflectionRegistry.h>

namespace SeedCore
{
	struct BlueprintField
	{
		String name_;

		AttributeType type_ = AttributeType::Unknown;

		Float floatValue_ = 0.0f;

		Vector2 vector2Value_ = Vector2(0.0f, 0.0f);

		Vector3 vector3Value_ = Vector3(0.0f, 0.0f, 0.0f);

		Vector4 vector4Value_ = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

		Color colorValue_ = Color(1.0f, 1.0f, 1.0f, 1.0f);

		Bool boolValue_ = false;

		Int intValue_ = 0;

		String stringValue_ = String();

		Bool isArray_ = false;

		DynamicArray<BlueprintField> children_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			Int typeValue = static_cast<Int>(type_);
			archive.Field("name", name_);
			archive.Field("type", typeValue);
			archive.Field("float", floatValue_);
			archive.Field("vector2", vector2Value_);
			archive.Field("vector3", vector3Value_);
			archive.Field("vector4", vector4Value_);
			archive.Field("color", colorValue_);
			archive.Field("bool", boolValue_);
			archive.Field("int", intValue_);
			archive.Field("string", stringValue_);
			archive.Field("is_array", isArray_);
			archive.Field("children", children_);
			type_ = static_cast<AttributeType>(typeValue);
		}
	};

	struct BlueprintComponent
	{
		String componentName_;

		DynamicArray<BlueprintField> fields_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("component", componentName_);
			archive.Field("fields", fields_);
		}

		SEEDCORE_API void Apply(void* componentData)const;

		SEEDCORE_API static BlueprintComponent Capture(const String& componentName, void* componentData);

	private:
		static void ApplyFields(const String& typeName, void* data, const DynamicArray<BlueprintField>& savedFields);

		static void ApplyScalar(const FieldInfo& field, void* ptr, const BlueprintField& match);

		static void CaptureFields(const String& typeName, void* data, DynamicArray<BlueprintField>& outFields);

		static BlueprintField CaptureScalar(const FieldInfo& field, void* ptr);

		static void Fields(const String& typeName, void* data, DynamicArray<FieldInfo>& fields);
	};
}
