#pragma once
#include <FoundationEngine/World/Command/Command.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	template <typename T>
	class ComponentCommand : public Command
	{
	public:
		ComponentCommand(World& world, Entity entity, ComponentID componentID, Size offset, const T& oldValue, const T& newValue) : world_(world), entity_(entity), componentID_(componentID), offset_(offset), oldValue_(oldValue), newValue_(newValue)
		{
			/// No Code
		}

		void Redo()override
		{
			Write(newValue_);
		}

		void Undo()override
		{
			Write(oldValue_);
		}

	private:
		void Write(const T& value)
		{
			void* component = world_.GetComponent(entity_, componentID_);
			if (!component)
			{
				return;
			}

			*reinterpret_cast<T*>(static_cast<Uint8*>(component) + offset_) = value;
		}

		World& world_;

		Entity entity_;

		ComponentID componentID_;

		Size offset_;

		T oldValue_;

		T newValue_;
	};

	template <typename T>
	class PointerCommand : public Command
	{
	public:
		PointerCommand(T* pointer, const T& oldValue, const T& newValue) : pointer_(pointer), oldValue_(oldValue), newValue_(newValue)
		{
			/// No Code
		}

		void Redo()override
		{
			*pointer_ = newValue_;
		}

		void Undo()override
		{
			*pointer_ = oldValue_;
		}

	private:
		T* pointer_;

		T oldValue_;

		T newValue_;
	};
}
