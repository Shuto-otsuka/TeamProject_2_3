#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeLocation.h>
#include <FoundationEngine/World/ECS/SparseSet/SparseSetLocation.h>

namespace SeedCore
{
	class Archetype;
	class Chunk;

	/**
	* [EN]
	* Everything recorded about one entity slot: its generation, whether it
	* is in use, and where its components are, in archetype storage and in
	* each sparse set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのエンティティスロットについて記録する全て。世代、使用中かどうか、
	* そしてコンポーネントがある場所（アーキタイプ格納と、各スパースセット）。
	*/
	struct EntityRecord
	{
		/// [EN] Current generation of this slot; an EntityID matches only while its generation_ equals this.
		/// [JP] このスロットの現在の世代。EntityID は generation_ がこれと等しい間だけ一致する。
		Uint32 generation_ = 0;

		/// [EN] Whether this slot is in use.
		/// [JP] このスロットが使用中かどうか。
		Bool alive_ = false;

		/// [EN] Where this entity's archetype-stored components are; one location covers all of them.
		/// [JP] このエンティティのアーキタイプ格納コンポーネントがある場所。1つの場所で全てを表す。
		ArchetypeLocation archetype_;

		/// [EN] Where each of this entity's sparse-stored components is, one location per component.
		/// [JP] このエンティティの各スパース格納コンポーネントがある場所。コンポーネントごとに1つ。
		DynamicArray<SparseSetLocation> sparseSet_;
	};

	/**
	* [EN]
	* Hands out entity slots and keeps one EntityRecord per slot, indexed
	* by EntityID::index_. A slot's generation advances on destroy, so
	* stale ids stop matching.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティのスロットを払い出し、スロットごとに EntityRecord を
	* 1つ保持する（EntityID::index_ で引く）。スロットの世代は破棄時に
	* 進むため、古い id は一致しなくなる。
	*/
	class SEEDCORE_API EntityRegistry
	{
	public:
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
		Entity Create();

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
		void Destroy(EntityID entityID);

		/**
		* [EN]
		* Returns whether entityID refers to a live slot of the same generation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID が同じ世代の生きたスロットを指しているかどうかを返す。
		*/
		Bool Exists(EntityID entityID)const;

		/**
		* [EN]
		* Returns entityID's record, or nullptr if entityID is stale; the
		* record is const when the registry is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のレコードを返す。entityID が古ければ nullptr。registry が
		* const ならレコードも const になる。
		*/
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
		/// [EN] One record per slot, indexed by EntityID::index_.
		/// [JP] スロットごとのレコード。EntityID::index_ で引く。
		DynamicArray<EntityRecord> records_;

		/// [EN] Freed slot indices, reused by Create before records_ grows.
		/// [JP] 空いたスロット番号。records_ を伸ばす前に Create が再利用する。
		DynamicArray<Uint32> freeSlots_;
	};
}