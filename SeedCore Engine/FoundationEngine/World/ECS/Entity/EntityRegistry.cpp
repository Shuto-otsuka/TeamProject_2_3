#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Allocates a slot, reusing a freed one if any, and returns an entity
	* for it with nothing placed yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スロットを確保し（空きがあれば再利用し）、まだ何も配置されていない
	* エンティティを返す。
	*/
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

	/**
	* [EN]
	* Resets entityID's record, advances its slot's generation and frees
	* the slot; does nothing if entityID is stale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID のレコードを初期化し、スロットの世代を進めて空きに戻す。
	* entityID が古ければ何もしない。
	*/
	void EntityRegistry::Destroy(EntityID entityID)
	{
		EntityRecord* record = Record(entityID);
		if (!record)
		{
			return;
		}

		record->archetype_ = {};

		/// [EN] clear keeps the capacity, so the entity that reuses this slot can add sparse components without reallocating.
		/// [JP] clear は容量を残すので、このスロットを再利用するエンティティはスパースコンポーネントを再確保なしで追加できる。
		record->sparseSet_.clear();

		record->alive_ = false;
		++record->generation_;
		freeSlots_.push_back(entityID.index_);
	}

	/**
	* [EN]
	* Returns whether entityID refers to a live slot of the same generation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID が同じ世代の生きたスロットを指しているかどうかを返す。
	*/
	Bool EntityRegistry::Exists(EntityID entityID)const
	{
		return Record(entityID) != nullptr;
	}
}