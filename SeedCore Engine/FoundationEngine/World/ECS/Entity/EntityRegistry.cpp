#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	Entity EntityRegistry::Create()
	{
		Uint32 index = 0;
		if (!freeSlots_.empty())
		{
			index = freeSlots_.back();
			freeSlots_.pop_back();
		}
		else
		{
			index = static_cast<Uint32>(records_.size());
			records_.emplace_back();
		}

		EntityRecord& record = records_[index];
		record.alive_ = true;
		return Entity(Handle<Entity>{index, record.generation_});
	}

	void EntityRegistry::Destroy(EntityID entityID)
	{
		EntityRecord* record = Record(entityID);
		if (!record)
		{
			return;
		}

		record->archetype_ = {};

		record->sparseSet_.clear();

		record->alive_ = false;
		++record->generation_;
		freeSlots_.push_back(entityID.index_);
	}

	Bool EntityRegistry::Exists(EntityID entityID)const
	{
		return Record(entityID) != nullptr;
	}
}