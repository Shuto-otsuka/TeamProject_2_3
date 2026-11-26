#pragma once
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	struct EntityID
	{
		Uint32 index_ = 0xFFFFFFFFu;

		Uint32 generation_ = 0;

		friend Bool operator==(const EntityID&, const EntityID&) = default;
	};

	class SEEDCORE_API Entity
	{
	public:
		Entity() = default;

		explicit Entity(Handle<Entity> handle);

		Bool operator==(const Entity& other)const;

		Bool operator!=(const Entity& other)const;

		const Handle<Entity>& GetHandle()const;

		EntityID GetID()const;

		Bool Exists()const;

		static Entity Null();

	private:
		Handle<Entity> handle_;
	};
}

template<>
struct std::hash<SeedCore::EntityID>
{
	SeedCore::Size operator()(const SeedCore::EntityID& id)const noexcept
	{
		SeedCore::Uint64 h = (static_cast<SeedCore::Uint64>(id.generation_) << 32) | static_cast<SeedCore::Uint64>(id.index_);
		h ^= h >> 33;
		h *= 0xFF51AFD7ED558CCDull;
		h ^= h >> 33;
		return static_cast<SeedCore::Size>(h);
	}
};
