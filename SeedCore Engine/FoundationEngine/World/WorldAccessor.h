#pragma once
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	class SEEDCORE_API WorldAccessor
	{
	public:
		static Actor Make(World& world, EntityID entityID);

		static Actor Create(World& world, String name, Uint32 persistentID);

		static Actor Find(World& world, Uint32 persistentID);

		static void Clear(World& world);

		static EntityRegistry& Entities(World& world);

		static ActorRegistry& Actors(World& world);

		static ArchetypeStorage& Archetypes(World& world);

		static SparseSetStorage& SparseSets(World& world);

		static Service& Services(World& world);
	};
}