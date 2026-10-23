#pragma once
#include <FoundationEngine/World/ECS/Archetype/Archetype.h>
#include <FoundationEngine/World/ECS/Archetype/Chunk.h>
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* One world's archetype-stored components, the counterpart of
	* SparseSetStorage: the chunks of every archetype in use. Adding and
	* removing move the entity along its archetype's cached edge and keep
	* each entity's ArchetypeLocation in the EntityRegistry up to date,
	* including the row of an entity moved by swap-remove. A chunk that
	* becomes empty goes back to its archetype's pool.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのワールドのアーキタイプ格納コンポーネントを持ち、SparseSetStorage と
	* 対になる。使われている各アーキタイプのチャンクを持つ。追加と削除では
	* アーキタイプのキャッシュ済み edge に沿ってエンティティを移し、
	* EntityRegistry にある各エンティティの ArchetypeLocation を最新に保つ
	* （swap-remove で動いたエンティティの行も含む）。空になったチャンクは
	* アーキタイプのプールへ返す。
	*/
	class SEEDCORE_API ArchetypeStorage :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Constructs an empty storage that records locations into entities.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 場所を entities へ記録する、空のストレージを構築する。
		*/
		ArchetypeStorage(EntityRegistry& entities);

		/**
		* [EN]
		* Returns every chunk to its archetype's pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全チャンクをそれぞれのアーキタイプのプールへ返す。
		*/
		~ArchetypeStorage();

		/**
		* [EN]
		* Moves entityID to the archetype with component id added,
		* default-constructing it, and returns it; returns the existing one if
		* entityID already has it, or nullptr if entityID is stale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID をコンポーネント id を足したアーキタイプへ移してそれを
		* デフォルト構築し、返す。既に持っていればそれを返し、entityID が
		* 古ければ nullptr。
		*/
		void* Add(EntityID entityID, ComponentID id);

		/**
		* [EN]
		* Moves entityID to the archetype with component id removed,
		* destroying it; removing the last one leaves entityID unplaced.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID をコンポーネント id を引いたアーキタイプへ移し、それを破棄
		* する。最後の1つを引いた場合、entityID は未配置になる。
		*/
		void Remove(EntityID entityID, ComponentID id);

		/**
		* [EN]
		* Destroys every archetype-stored component of entityID, leaving it
		* unplaced.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID の全アーキタイプ格納コンポーネントを破棄し、未配置にする。
		*/
		void Clear(EntityID entityID);

		/**
		* [EN]
		* Returns entityID's component id, or nullptr if it has none.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のコンポーネント id を返す。持っていなければ nullptr。
		*/
		void* Data(EntityID entityID, ComponentID id)const;

		/**
		* [EN]
		* Returns whether entityID has component id.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID がコンポーネント id を持っているかどうかを返す。
		*/
		Bool Has(EntityID entityID, ComponentID id)const;

		/**
		* [EN]
		* Returns the chunks of archetype in this world; empty if none.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このワールドにある archetype のチャンクを返す。無ければ空。
		*/
		std::span<Chunk* const> Chunks(Archetype* archetype)const;

		/**
		* [EN]
		* Returns the archetype-stored components of entityID; empty if it is
		* unplaced or stale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のアーキタイプ格納コンポーネントを返す。未配置か古ければ空。
		*/
		const DynamicArray<ComponentID>& Layout(EntityID entityID)const;

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
		Uint32 Version()const;

	private:
		/**
		* [EN]
		* Finishes vacating row of chunk, a chunk of archetype that has just
		* released the row: fixes the row of movedEntityID (the entity Release
		* moved inside chunk), fills the gap at chunk's end from the
		* archetype's last chunk so only the last chunk can have free rows,
		* and returns the last chunk to the pool once it is empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archetype のチャンク chunk が row を解放した直後に、空けた後始末を
		* 仕上げる。movedEntityID（Release が chunk 内で動かしたエンティティ）の
		* 行を直し、chunk の末尾の空きをアーキタイプの最後のチャンクから埋めて
		* 空きが最後のチャンクにしか無いようにし、最後のチャンクが空になったら
		* プールへ返す。
		*/
		void Compact(Archetype* archetype, Chunk* chunk, Uint32 row, EntityID movedEntityID);

	private:
		/// [EN] Where each entity's archetype location is recorded; owned by the world.
		/// [JP] 各エンティティのアーキタイプの場所を記録する先。ワールドが所有する。
		EntityRegistry& entities_;

		/// [EN] The chunks of each archetype in use in this world, indexed by the archetype's index; the last one is the only one that may have free rows.
		/// [JP] このワールドで使われている各アーキタイプのチャンク。アーキタイプの番号で引く。空きの行がありうるのは最後のチャンクだけ。
		DynamicArray<DynamicArray<Chunk*>> chunks_;

		/// [EN] Advanced by every add and remove.
		/// [JP] 追加・削除のたびに進む。
		Uint32 version_ = 0;
	};
}