#pragma once
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	struct ActorRecord
	{
		EntityID parent_;

		EntityID child_;

		EntityID nextSibling_;

		EntityID previousSibling_;

		Uint64 tags_ = 0;

		ResourcePtr<Bitset> extraTags_;

		Uint32 layer_ = 0;

		Uint32 persistentID_ = 0;
	};

	class SEEDCORE_API ActorRegistry :public NonTransferable
	{
	public:
		ActorRegistry(EntityRegistry& entities);

		void Add(EntityID entityID, Uint32 persistentID);

		void Remove(EntityID entityID);

		void Attach(EntityID child, EntityID parent);

		void Move(EntityID child, EntityID after);

		EntityID Root()const;

		EntityID Next(EntityID entityID)const;

		EntityID Find(Uint32 persistentID)const;

		template<typename Self>
		auto Record(this Self& self, EntityID entityID) -> decltype(&self.records_[0])
		{
			if (!self.entities_.Exists(entityID) || entityID.index_ >= self.records_.size())
			{
				return nullptr;
			}
			return &self.records_[entityID.index_];
		}

	private:
		void Link(EntityID child, EntityID parent, EntityID after);

		void Unlink(EntityID child);

	private:
		EntityRegistry& entities_;

		DynamicArray<ActorRecord> records_;

		EntityID root_;

		DynamicArray<EntityID> persistentIndex_;

		Uint32 nextPersistentID_ = 1;
	};
}