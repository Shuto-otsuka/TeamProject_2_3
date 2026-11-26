#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct ArchetypeEdge;

	class Archetype;

	class SEEDCORE_API Chunk
	{
	public:
		Chunk(Archetype* archetype);

		~Chunk();

		Uint32 Add(EntityID entityID);

		EntityID Remove(Uint32 row);

		EntityID Release(Uint32 row);

		Uint32 Adopt(Chunk& source, Uint32 sourceRow);

		Uint32 Adopt(Chunk& source, Uint32 sourceRow, const ArchetypeEdge& edge);

		Uint8* Data(Size layoutIndex);

		Size Length(Size layoutIndex)const;

		Bool Full()const;

		Size Capacity()const;

		EntityID EntityAt(Size index)const;

		Uint32 EntityCount()const;

	private:
		SC_CONSTEX Size chunkSize_ = 16 * 1024;

		SC_CONSTEX Size maxEntitiesPerChunk_ = 1024;

		SC_CONSTEX Size maxComponentsPerArchetype_ = 32;

		Archetype* archetype_;

		Uint8* componentDataPointers_[maxComponentsPerArchetype_];

		Size componentSizes_[maxComponentsPerArchetype_];

		Size componentCount_ = 0;

		Size capacity_ = 0;

		alignas(64) Uint8 buffer_[chunkSize_];

		Uint32 entityCount_ = 0;

		alignas(64) EntityID entityIDs_[maxEntitiesPerChunk_];
	};
}
