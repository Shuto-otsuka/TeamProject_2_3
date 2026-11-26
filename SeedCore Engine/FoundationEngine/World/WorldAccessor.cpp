#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	Actor WorldAccessor::Make(World& world, EntityID entityID)
	{
		if (!world.entities_.Exists(entityID))
		{
			return Actor();
		}
		return ActorAccessor::Make(world, Entity(Handle<Entity>{entityID.index_, entityID.generation_}));
	}

	Actor WorldAccessor::Create(World& world, String name, Uint32 persistentID)
	{
		return world.CreateActor(name, persistentID);
	}

	Actor WorldAccessor::Find(World& world, Uint32 persistentID)
	{
		return Make(world, world.actors_.Find(persistentID));
	}

	void WorldAccessor::Clear(World& world)
	{
		ActorRegistry& actors = world.actors_;
		for (EntityID root = actors.Root();root != EntityID{};root = actors.Root())
		{
			world.DestroyActor(Make(world, root));
		}
	}

	EntityRegistry& WorldAccessor::Entities(World& world)
	{
		return world.entities_;
	}

	ActorRegistry& WorldAccessor::Actors(World& world)
	{
		return world.actors_;
	}

	ArchetypeStorage& WorldAccessor::Archetypes(World& world)
	{
		return world.archetypes_;
	}

	SparseSetStorage& WorldAccessor::SparseSets(World& world)
	{
		return world.sparseSets_;
	}

	Service& WorldAccessor::Services(World& world)
	{
		return world.service_;
	}
}