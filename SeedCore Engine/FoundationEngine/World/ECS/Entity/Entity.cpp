#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	Entity::Entity(Handle<Entity> handle) : handle_(handle)
	{
		/// No Code
	}

	Bool Entity::operator==(const Entity& other)const
	{
		return handle_ == other.handle_;
	}

	Bool Entity::operator!=(const Entity& other)const
	{
		return handle_ != other.handle_;
	}

	const Handle<Entity>& Entity::GetHandle()const
	{
		return handle_;
	}

	EntityID Entity::GetID()const
	{
		if (!handle_.exists())
		{
			return EntityID{};
		}

		return EntityID{ static_cast<Uint32>(handle_.index_), static_cast<Uint32>(handle_.generation_) };
	}

	Bool Entity::Exists()const
	{
		return handle_.exists();
	}

	Entity Entity::Null()
	{
		return Entity(Handle<Entity>::null());
	}
}
