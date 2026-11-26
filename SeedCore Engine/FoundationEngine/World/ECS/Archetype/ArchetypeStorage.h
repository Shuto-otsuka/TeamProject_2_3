#pragma once
#include <FoundationEngine/World/ECS/Archetype/Archetype.h>
#include <FoundationEngine/World/ECS/Archetype/Chunk.h>
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	class SEEDCORE_API ArchetypeStorage :public NonTransferable
	{
	public:
		ArchetypeStorage(EntityRegistry& entities);

		~ArchetypeStorage();

		void* Add(EntityID entityID, ComponentID id);

		void Remove(EntityID entityID, ComponentID id);

		void Clear(EntityID entityID);

		void* Data(EntityID entityID, ComponentID id)const;

		Bool Has(EntityID entityID, ComponentID id)const;

		std::span<Chunk* const> Chunks(Archetype* archetype)const;

		const DynamicArray<ComponentID>& Layout(EntityID entityID)const;

		Uint32 Version()const;

	private:
		void Compact(Archetype* archetype, Chunk* chunk, Uint32 row, EntityID movedEntityID);

	private:
		EntityRegistry& entities_;

		DynamicArray<DynamicArray<Chunk*>> chunks_;

		Uint32 version_ = 0;
	};
}