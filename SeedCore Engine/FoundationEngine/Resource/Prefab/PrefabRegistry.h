#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	/**
	* [EN]
	* What the prefab system records about one actor it instantiated: which
	* .prefab asset an instance's root came from, and whether the actor was
	* built from a prefab at all.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プレハブの仕組みが、自分でインスタンス化した actor 1体について記録する
	* もの。インスタンスのルートがどの .prefab アセットから来たかと、その
	* actor がそもそもプレハブから作られたかどうか。
	*/
	struct PrefabRecord
	{
		/// [EN] The entity this record was written for, generation included; a record whose entity differs from the one asked for is stale.
		/// [JP] このレコードを書いたときのエンティティ（世代込み）。尋ねられたものと違えば、そのレコードは古い。
		EntityID entity_;

		/// [EN] The .prefab asset an instance's root came from; 0 for every other actor.
		/// [JP] インスタンスのルートが来た .prefab アセット。それ以外の actor では 0。
		Uint32 assetID_ = 0;

		/// [EN] Whether the actor was built from a prefab, root or descendant alike.
		/// [JP] actor がプレハブから作られたかどうか。ルートか子孫かは問わない。
		Bool fromPrefab_ = false;
	};

	/**
	* [EN]
	* One world's links from actors to the prefabs they were instantiated
	* from, kept as a World service so neither the actor's record nor a
	* component carries them. One PrefabRecord per entity slot, indexed by
	* EntityID::index_. A record is never cleared when its actor is
	* destroyed: once the slot is reused the generation no longer matches,
	* so Record treats it as absent. Only actors the prefab system writes
	* take up records, and a world that never instantiates a prefab never
	* has this service at all.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのワールドの、actor からインスタンス化元のプレハブへのつながり。
	* actor のレコードにもコンポーネントにも持たせず、World のサービスとして
	* 持つ。エンティティのスロットごとに PrefabRecord を1つ持ち、
	* EntityID::index_ で引く。actor が破棄されてもレコードは消さない。
	* スロットが再利用されれば世代が合わなくなり、Record は無いものとして
	* 扱うため。レコードを使うのはプレハブの仕組みが書いた actor だけで、
	* プレハブを一度もインスタンス化しないワールドには、このサービス自体が
	* 無い。
	*/
	class SEEDCORE_API PrefabRegistry :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Writes entityID's record, replacing whatever its slot held.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のレコードを書く。そのスロットにあったものは置き換える。
		*/
		void Add(EntityID entityID, Uint32 assetID, Bool fromPrefab);

		/**
		* [EN]
		* Returns entityID's record, or nullptr if none was written for it
		* (or the one in its slot belongs to an earlier entity); the record is
		* const when the registry is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のレコードを返す。書かれていなければ（またはスロットに
		* あるのが前のエンティティのものなら）nullptr。registry が const なら
		* レコードも const になる。
		*/
		template<typename Self>
		auto Record(this Self& self, EntityID entityID) -> decltype(&self.records_[0])
		{
			if (entityID.index_ >= self.records_.size() || self.records_[entityID.index_].entity_ != entityID)
			{
				return nullptr;
			}
			return &self.records_[entityID.index_];
		}

	private:
		/// [EN] One record per entity slot, indexed by EntityID::index_; grows only as far as the highest slot written.
		/// [JP] エンティティのスロットごとのレコード。EntityID::index_ で引く。書かれた一番大きいスロットまでしか伸びない。
		DynamicArray<PrefabRecord> records_;
	};
}