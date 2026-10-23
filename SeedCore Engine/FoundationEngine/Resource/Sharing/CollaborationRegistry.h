#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	/**
	* [EN]
	* What collaborative editing records about one actor: the identifier
	* every member's copy of the scene shares for it, so an arriving scene
	* can be matched against the actors already live.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共同編集が actor 1体について記録するもの。各メンバーのシーンで共通の
	* その actor の識別子で、届いたシーンを既にいる actor と突き合わせる
	* ために使う。
	*/
	struct CollaborationRecord
	{
		/// [EN] The entity this record was written for, generation included; a record whose entity differs from the one asked for is stale.
		/// [JP] このレコードを書いたときのエンティティ（世代込み）。尋ねられたものと違えば、そのレコードは古い。
		EntityID entity_;

		/// [EN] The identifier shared across every member's copy of the scene.
		/// [JP] 各メンバーのシーンで共通の識別子。
		String id_;
	};

	/**
	* [EN]
	* One world's collaboration identifiers, kept as a World service so
	* neither the actor's record nor a component carries them. One
	* CollaborationRecord per entity slot, indexed by EntityID::index_. A
	* record is never cleared when its actor is destroyed: once the slot is
	* reused the generation no longer matches, so Record treats it as
	* absent. Only actors that hold an identifier take up records, and a
	* world that never loads one never has this service at all.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのワールドの共同編集の識別子。actor のレコードにもコンポーネント
	* にも持たせず、World のサービスとして持つ。エンティティのスロットごとに
	* CollaborationRecord を1つ持ち、EntityID::index_ で引く。actor が破棄
	* されてもレコードは消さない。スロットが再利用されれば世代が合わなく
	* なり、Record は無いものとして扱うため。レコードを使うのは識別子を持つ
	* actor だけで、一度も識別子を読み込まないワールドには、このサービス
	* 自体が無い。
	*/
	class SEEDCORE_API CollaborationRegistry :public NonTransferable
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
		void Add(EntityID entityID, String id);

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
		DynamicArray<CollaborationRecord> records_;
	};
}