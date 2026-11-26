#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	class Archetype;
	class Chunk;

	struct ArchetypeEdge
	{
		Archetype* target_ = nullptr;

		DynamicArray<Uint32> sourceColumns_;

		Uint32 removedColumn_ = UINT32_MAX;
	};

	class SEEDCORE_API Archetype
	{
	public:
		Archetype(const DynamicArray<ComponentID>& layout, Uint32 index);

		~Archetype() = default;

		const DynamicArray<ComponentID>& Layout()const;

		Uint32 Index()const;

		const Bitset& Signature()const;

		const ArchetypeEdge& With(ComponentID id);

		const ArchetypeEdge& Without(ComponentID id);

		Chunk* CreateChunk();

		void RecycleChunk(Chunk* chunk);

	private:
		DynamicArray<ComponentID> layout_;

		Uint32 index_ = 0;

		Bitset signature_;

		DynamicArray<ResourcePtr<ArchetypeEdge>> withEdges_;

		DynamicArray<ResourcePtr<ArchetypeEdge>> withoutEdges_;

		ObjectPool<Chunk> chunkPool_;
	};
}
