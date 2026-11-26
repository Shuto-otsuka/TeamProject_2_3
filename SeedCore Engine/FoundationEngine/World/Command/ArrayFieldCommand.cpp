#include <FoundationEngine/World/Command/ArrayFieldCommand.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>

namespace SeedCore
{
	ArrayFieldCommand::ArrayFieldCommand(World& world, Entity entity, ComponentID componentID, String fieldName, Size index) : world_(world), entity_(entity), componentID_(componentID), fieldName_(fieldName), index_(index)
	{
		/// No Code
	}

	ArrayAppendCommand::ArrayAppendCommand(World& world, Entity entity, ComponentID componentID, String fieldName, Size index) : ArrayFieldCommand(world, entity, componentID, fieldName, index)
	{
		/// No Code
	}

	void ArrayAppendCommand::Redo()
	{
		void* component = world_.GetComponent(entity_, componentID_);
		if (!component)
		{
			return;
		}

		auto& registry = ReflectionRegistry::GetRegistry();
		auto reflector = registry.find(ComponentRegistry::Name(componentID_));
		if (reflector == registry.end())
		{
			return;
		}

		DynamicArray<FieldInfo> fields;
		reflector->second(component, fields);

		for (const FieldInfo& field : fields)
		{
			if (field.name_ == fieldName_ && field.array_.add_)
			{
				field.array_.add_();
				return;
			}
		}
	}

	void ArrayAppendCommand::Undo()
	{
		void* component = world_.GetComponent(entity_, componentID_);
		if (!component)
		{
			return;
		}

		auto& registry = ReflectionRegistry::GetRegistry();
		auto reflector = registry.find(ComponentRegistry::Name(componentID_));
		if (reflector == registry.end())
		{
			return;
		}

		DynamicArray<FieldInfo> fields;
		reflector->second(component, fields);

		for (const FieldInfo& field : fields)
		{
			if (field.name_ == fieldName_ && field.array_.remove_)
			{
				field.array_.remove_(index_);
				return;
			}
		}
	}

	PayloadArrayCommand::PayloadArrayCommand(World& world, Entity entity, ComponentID componentID, String fieldName, Size index, Int value, Bool addOnRedo) : ArrayFieldCommand(world, entity, componentID, fieldName, index), value_(value), addOnRedo_(addOnRedo)
	{
		/// No Code
	}

	void PayloadArrayCommand::Redo()
	{
		void* component = world_.GetComponent(entity_, componentID_);
		if (!component)
		{
			return;
		}

		auto& registry = ReflectionRegistry::GetRegistry();
		auto reflector = registry.find(ComponentRegistry::Name(componentID_));
		if (reflector == registry.end())
		{
			return;
		}

		DynamicArray<FieldInfo> fields;
		reflector->second(component, fields);

		for (const FieldInfo& field : fields)
		{
			if (field.name_ != fieldName_ || !field.array_.add_)
			{
				continue;
			}

			if (addOnRedo_)
			{
				field.array_.add_();
				if (field.array_.lastPtr_)
				{
					void* element = field.array_.lastPtr_();
					if (element)
					{
						*static_cast<Int*>(element) = value_;
					}
				}
			}
			else if (field.array_.remove_)
			{
				field.array_.remove_(index_);
			}
			return;
		}
	}

	void PayloadArrayCommand::Undo()
	{
		void* component = world_.GetComponent(entity_, componentID_);
		if (!component)
		{
			return;
		}

		auto& registry = ReflectionRegistry::GetRegistry();
		auto reflector = registry.find(ComponentRegistry::Name(componentID_));
		if (reflector == registry.end())
		{
			return;
		}

		DynamicArray<FieldInfo> fields;
		reflector->second(component, fields);

		for (const FieldInfo& field : fields)
		{
			if (field.name_ != fieldName_ || !field.array_.add_)
			{
				continue;
			}

			if (addOnRedo_)
			{
				if (field.array_.remove_)
				{
					field.array_.remove_(index_);
				}
			}
			else
			{
				field.array_.add_();
				if (field.array_.lastPtr_)
				{
					void* element = field.array_.lastPtr_();
					if (element)
					{
						*static_cast<Int*>(element) = value_;
					}
				}
			}
			return;
		}
	}
}
