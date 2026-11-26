#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>

namespace SeedCore
{
	void BlueprintComponent::Apply(void* componentData)const
	{
		ApplyFields(componentName_, componentData, fields_);
	}

	BlueprintComponent BlueprintComponent::Capture(const String& componentName, void* componentData)
	{
		BlueprintComponent serializedComponent{};
		serializedComponent.componentName_ = componentName;
		CaptureFields(componentName, componentData, serializedComponent.fields_);
		return serializedComponent;
	}

	void BlueprintComponent::ApplyFields(const String& typeName, void* data, const DynamicArray<BlueprintField>& savedFields)
	{
		if (savedFields.empty())
		{
			return;
		}

		{
			DynamicArray<FieldInfo> fields;
			Fields(typeName, data, fields);

			for (const FieldInfo& field : fields)
			{
				if (!field.array_.add_)
				{
					continue;
				}

				auto matchIt = std::ranges::find(savedFields, field.name_, &BlueprintField::name_);
				const BlueprintField* match = matchIt != savedFields.end() ? &*matchIt : nullptr;
				if (!match)
				{
					continue;
				}

				Size currentCount = field.array_.size_;
				while (currentCount < match->children_.size())
				{
					field.array_.add_();
					++currentCount;
				}
			}
		}

		DynamicArray<FieldInfo> fields;
		Fields(typeName, data, fields);

		for (Size index = 0; index < fields.size(); ++index)
		{
			const FieldInfo& field = fields[index];

			auto matchIt = std::ranges::find(savedFields, field.name_, &BlueprintField::name_);
			const BlueprintField* match = matchIt != savedFields.end() ? &*matchIt : nullptr;

			if (field.array_.size_ > 0 || field.array_.add_)
			{
				Size count = field.array_.size_;

				if (match)
				{
					for (Size element = 0; element < count && element < match->children_.size() && (index + 1 + element) < fields.size(); ++element)
					{
						const FieldInfo& elementField = fields[index + 1 + element];
						void* ptr = elementField.directPtr_ ? elementField.directPtr_ : (static_cast<Uint8*>(data) + elementField.offset_);
						const BlueprintField& savedElement = match->children_[element];

						if (!elementField.nestedTypeName_.view().empty())
						{
							ApplyFields(elementField.nestedTypeName_, ptr, savedElement.children_);
						}
						else
						{
							ApplyScalar(elementField, ptr, savedElement);
						}
					}
				}

				index += count;
				continue;
			}

			if (!match)
			{
				continue;
			}

			void* ptr = field.directPtr_ ? field.directPtr_ : (static_cast<Uint8*>(data) + field.offset_);

			if (!field.nestedTypeName_.view().empty())
			{
				ApplyFields(field.nestedTypeName_, ptr, match->children_);
				continue;
			}

			ApplyScalar(field, ptr, *match);
		}
	}

	void BlueprintComponent::ApplyScalar(const FieldInfo& field, void* ptr, const BlueprintField& match)
	{
		if (match.type_ != field.type_)
		{
			return;
		}

		switch (field.type_)
		{
		case AttributeType::Int:
		case AttributeType::Enum:
			*static_cast<Int*>(ptr) = match.intValue_;
			break;
		case AttributeType::Float:
			*static_cast<Float*>(ptr) = match.floatValue_;
			break;
		case AttributeType::Bool:
			*static_cast<Bool*>(ptr) = match.boolValue_;
			break;
		case AttributeType::Vector2:
			*static_cast<Vector2*>(ptr) = match.vector2Value_;
			break;
		case AttributeType::Vector3:
			*static_cast<Vector3*>(ptr) = match.vector3Value_;
			break;
		case AttributeType::Vector4:
			*static_cast<Vector4*>(ptr) = match.vector4Value_;
			break;
		case AttributeType::String:
			*static_cast<String*>(ptr) = match.stringValue_;
			break;
		case AttributeType::Color:
			*static_cast<Color*>(ptr) = match.colorValue_;
			break;
		default:
			break;
		}
	}

	void BlueprintComponent::CaptureFields(const String& typeName, void* data, DynamicArray<BlueprintField>& outFields)
	{
		DynamicArray<FieldInfo> fields;
		Fields(typeName, data, fields);

		for (Size index = 0; index < fields.size(); ++index)
		{
			const FieldInfo& field = fields[index];

			if (field.array_.size_ > 0 || field.array_.add_)
			{
				Size count = field.array_.size_;

				BlueprintField arrayField;
				arrayField.name_ = field.name_;
				arrayField.isArray_ = true;

				for (Size element = 0; element < count && (index + 1 + element) < fields.size(); ++element)
				{
					const FieldInfo& elementField = fields[index + 1 + element];
					void* ptr = elementField.directPtr_ ? elementField.directPtr_ : (static_cast<Uint8*>(data) + elementField.offset_);

					if (!elementField.nestedTypeName_.view().empty())
					{
						BlueprintField structField;
						CaptureFields(elementField.nestedTypeName_, ptr, structField.children_);
						arrayField.children_.push_back(std::move(structField));
					}
					else
					{
						arrayField.children_.push_back(CaptureScalar(elementField, ptr));
					}
				}

				outFields.push_back(std::move(arrayField));
				index += count;
				continue;
			}

			void* ptr = field.directPtr_ ? field.directPtr_ : (static_cast<Uint8*>(data) + field.offset_);

			if (!field.nestedTypeName_.view().empty())
			{
				BlueprintField structField;
				structField.name_ = field.name_;
				CaptureFields(field.nestedTypeName_, ptr, structField.children_);
				outFields.push_back(std::move(structField));
				continue;
			}

			outFields.push_back(CaptureScalar(field, ptr));
		}
	}

	BlueprintField BlueprintComponent::CaptureScalar(const FieldInfo& field, void* ptr)
	{
		BlueprintField capturedField;
		capturedField.name_ = field.name_;
		capturedField.type_ = field.type_;

		switch (field.type_)
		{
		case AttributeType::Int:
		case AttributeType::Enum:
			capturedField.intValue_ = *static_cast<Int*>(ptr);
			break;
		case AttributeType::Float:
			capturedField.floatValue_ = *static_cast<Float*>(ptr);
			break;
		case AttributeType::Bool:
			capturedField.boolValue_ = *static_cast<Bool*>(ptr);
			break;
		case AttributeType::Vector2:
			capturedField.vector2Value_ = *static_cast<Vector2*>(ptr);
			break;
		case AttributeType::Vector3:
			capturedField.vector3Value_ = *static_cast<Vector3*>(ptr);
			break;
		case AttributeType::Vector4:
			capturedField.vector4Value_ = *static_cast<Vector4*>(ptr);
			break;
		case AttributeType::String:
			capturedField.stringValue_ = *static_cast<String*>(ptr);
			break;
		case AttributeType::Color:
			capturedField.colorValue_ = *static_cast<Color*>(ptr);
			break;
		default:
			break;
		}

		return capturedField;
	}

	void BlueprintComponent::Fields(const String& typeName, void* data, DynamicArray<FieldInfo>& fields)
	{
		auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
		auto reflectionIt = reflectionRegistry.find(typeName);
		if (reflectionIt != reflectionRegistry.end())
		{
			reflectionIt->second(data, fields);
		}

		auto& payloadRegistry = PayloadRegistry::GetRegistry();
		auto payloadIt = payloadRegistry.find(typeName);
		if (payloadIt != payloadRegistry.end())
		{
			payloadIt->second(data, fields);
		}
	}
}
