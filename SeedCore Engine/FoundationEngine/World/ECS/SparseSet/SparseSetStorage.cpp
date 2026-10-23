#include <FoundationEngine/World/ECS/SparseSet/SparseSetStorage.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs an empty storage that records locations into entities.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 場所を entities へ記録する、空のストレージを構築する。
	*/
	SparseSetStorage::SparseSetStorage(EntityRegistry& entities) :entities_(entities)
	{
		/// No Code
	}

	/**
	* [EN]
	* Default-constructs component id for entityID and returns it; returns
	* the existing one if entityID already has it, or nullptr if entityID
	* is stale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID にコンポーネント id をデフォルト構築して返す。既に持って
	* いればそれを返し、entityID が古ければ nullptr。
	*/
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

		/// [EN] A set from an earlier registration of the same component may differ in size, so it is replaced; every entity has already dropped it, so it is empty.
		/// [JP] 同じコンポーネントの以前の登録のセットはサイズが違うかもしれないので作り直す。全エンティティが既に外しているので、中は空。
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

	/**
	* [EN]
	* Destroys entityID's component id, if it has one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID がコンポーネント id を持っていれば破棄する。
	*/
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

		/// [EN] The entity moved into the vacated row keeps its component's address, so only its row is updated.
		/// [JP] 空いた行へ動いてきたエンティティのコンポーネントはアドレスが変わらないので、行だけを直す。
		EntityID movedEntityID = sets_[id->index_]->Remove(row);
		if (movedEntityID != EntityID{})
		{
			EntityRecord* movedRecord = entities_.Record(movedEntityID);
			auto movedIt = std::ranges::find(movedRecord->sparseSet_, id, &SparseSetLocation::id_);
			movedIt->row_ = row;
		}

		++version_;
	}

	/**
	* [EN]
	* Destroys every sparse-stored component of entityID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID の全スパース格納コンポーネントを破棄する。
	*/
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

	/**
	* [EN]
	* Returns entityID's component id, or nullptr if it has none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID のコンポーネント id を返す。持っていなければ nullptr。
	*/
	void* SparseSetStorage::Data(EntityID entityID, ComponentID id)const
	{
		const EntityRecord* record = entities_.Record(entityID);
		if (!record)
		{
			return nullptr;
		}

		/// [EN] The location holds the component's address itself, so the sparse set is not touched.
		/// [JP] 場所がコンポーネントのアドレスそのものを持っているので、スパースセットには触れない。
		for (const SparseSetLocation& location : record->sparseSet_)
		{
			if (location.id_ == id)
			{
				return location.data_;
			}
		}
		return nullptr;
	}

	/**
	* [EN]
	* Returns whether entityID has component id.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID がコンポーネント id を持っているかどうかを返す。
	*/
	Bool SparseSetStorage::Has(EntityID entityID, ComponentID id)const
	{
		const EntityRecord* record = entities_.Record(entityID);
		if (!record)
		{
			return false;
		}
		return std::ranges::contains(record->sparseSet_, id, &SparseSetLocation::id_);
	}

	/**
	* [EN]
	* Returns the sparse set of component id, or nullptr if nothing has
	* added it under its current registration yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネント id のスパースセットを返す。今の登録でまだ誰も
	* 追加していなければ nullptr。
	*/
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

	/**
	* [EN]
	* Returns how many times adding or removing has changed this storage;
	* a traversal compares it before and after each step to catch a change
	* that would move the rows it is iterating.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 追加・削除でこのストレージが変わった回数を返す。走査はこれを各段階の
	* 前後で比べ、走査中の行を動かす変更を見つける。
	*/
	Uint32 SparseSetStorage::Version()const
	{
		return version_;
	}
}