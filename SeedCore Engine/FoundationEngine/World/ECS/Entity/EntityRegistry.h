#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeLocation.h>
#include <FoundationEngine/World/ECS/SparseSet/SparseSetLocation.h>

namespace SeedCore
{
	class Archetype;
	class Chunk;

	struct EntityRecord
	{
		Uint32 generation_ = 0;

		Bool alive_ = false;

		ArchetypeLocation archetype_;

		DynamicArray<SparseSetLocation> sparseSet_;
	};

	class SEEDCORE_API EntityRegistry
	{
	public:
		Entity Create();

		void Destroy(EntityID entityID);

		Bool Exists(EntityID entityID)const;

		template<typename Self>
		auto Record(this Self& self, EntityID entityID) -> decltype(&self.records_[0])
		{
			if (entityID.index_ >= self.records_.size())
			{
				return nullptr;
			}

			auto& record = self.records_[entityID.index_];
			if (!record.alive_ || record.generation_ != entityID.generation_)
			{
				return nullptr;
			}
			return &record;
		}

	private:
		DynamicArray<EntityRecord> records_;

		DynamicArray<Uint32> freeSlots_;
	};
}