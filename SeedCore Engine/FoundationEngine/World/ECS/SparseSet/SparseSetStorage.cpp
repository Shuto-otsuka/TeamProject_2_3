#include <FoundationEngine/World/ECS/SparseSet/SparseSetStorage.h>

namespace SeedCore
{
	SparseSetStorage::SparseSetStorage(EntityRegistry& entities) :entities_(entities)
	{
		/// No Code
	}

	void* SparseSetStorage::Add(EntityID entityID, ComponentID id)
	{
		EntityRecord* record = entities_.Record(entityID);
		if (!record || !id)
		{
			return nullptr;
		}

		for (const SparseSetLocation& location : record->sparseSet_)
		{
			if (location.id_ == id)
			{
				return location.data_;
			}
		}

		if (id->index_ >= sets_.size())
		{
			sets_.resize(id->index_ + 1);
		}

		ResourcePtr<SparseSet>& set = sets_[id->index_];
		if (!set || set->Epoch() != id->epoch_)
		{
			SC_ASSERT(!set || set->EntityCount() == 0, "登録解除されたコンポーネントのスパースセットに、まだエンティティが残っています。登録解除の前に全エンティティから外してください。");
			set = MakePtr<SparseSet>(id);
		}

		Uint32 row = set->Add(entityID);
		void* data = set->Data(row);
		record->sparseSet_.push_back(SparseSetLocation{ id,row,data });

		++version_;
		return data;
	}

	void SparseSetStorage::Remove(EntityID entityID, ComponentID id)
	{
		EntityRecord* record = entities_.Record(entityID);
		if (!record)
		{
			return;
		}

		auto it = std::ranges::find(record->sparseSet_, id, &SparseSetLocation::id_);
		if (it == record->sparseSet_.end())
		{
			return;
		}

		Uint32 row = it->row_;
		record->sparseSet_.erase(it);

		EntityID movedEntityID = sets_[id->index_]->Remove(row);
		if (movedEntityID != EntityID{})
		{
			EntityRecord* movedRecord = entities_.Record(movedEntityID);
			auto movedIt = std::ranges::find(movedRecord->sparseSet_, id, &SparseSetLocation::id_);
			movedIt->row_ = row;
		}

		++version_;
	}

	void SparseSetStorage::Clear(EntityID entityID)
	{
		EntityRecord* record = entities_.Record(entityID);
		if (!record || record->sparseSet_.empty())
		{
			return;
		}

		for (const SparseSetLocation& location : record->sparseSet_)
		{
			EntityID movedEntityID = sets_[location.id_->index_]->Remove(location.row_);
			if (movedEntityID != EntityID{})
			{
				EntityRecord* movedRecord = entities_.Record(movedEntityID);
				auto movedIt = std::ranges::find(movedRecord->sparseSet_, location.id_, &SparseSetLocation::id_);
				movedIt->row_ = location.row_;
			}
		}

		record->sparseSet_.clear();
		++version_;
	}

	void* SparseSetStorage::Data(EntityID entityID, ComponentID id)const
	{
		const EntityRecord* record = entities_.Record(entityID);
		if (!record)
		{
			return nullptr;
		}

		for (const SparseSetLocation& location : record->sparseSet_)
		{
			if (location.id_ == id)
			{
				return location.data_;
			}
		}
		return nullptr;
	}

	Bool SparseSetStorage::Has(EntityID entityID, ComponentID id)const
	{
		const EntityRecord* record = entities_.Record(entityID);
		if (!record)
		{
			return false;
		}
		return std::ranges::contains(record->sparseSet_, id, &SparseSetLocation::id_);
	}

	SparseSet* SparseSetStorage::Set(ComponentID id)const
	{
		if (!id || id->index_ >= sets_.size())
		{
			return nullptr;
		}

		const ResourcePtr<SparseSet>& set = sets_[id->index_];
		if (!set || set->Epoch() != id->epoch_)
		{
			return nullptr;
		}
		return set.get();
	}

	Uint32 SparseSetStorage::Version()const
	{
		return version_;
	}
}