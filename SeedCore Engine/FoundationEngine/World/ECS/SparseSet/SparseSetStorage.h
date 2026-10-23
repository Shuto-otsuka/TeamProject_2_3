#pragma once
#include <FoundationEngine/World/ECS/SparseSet/SparseSet.h>
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* One world's sparse-stored components, the counterpart of
	* ArchetypeStorage: one SparseSet per component, indexed by
	* ComponentRecord::index_. Adding and removing keep each entity's
	* SparseSetLocation list in the EntityRegistry up to date, including
	* the row of an entity moved by swap-remove. A set left over from an
	* earlier registration of the same component is recognized by its
	* epoch and replaced on the next Add.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのワールドのスパース格納コンポーネントを持ち、ArchetypeStorage と
	* 対になる。コンポーネントごとに SparseSet を1つ持ち、
	* ComponentRecord::index_ で引く。追加と削除では、EntityRegistry にある
	* 各エンティティの SparseSetLocation 一覧を最新に保つ（swap-remove で
	* 動いたエンティティの行も含む）。同じコンポーネントの以前の登録から
	* 残ったセットは epoch で見分け、次の Add で作り直す。
	*/
	class SEEDCORE_API SparseSetStorage :public NonTransferable
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
		SparseSetStorage(EntityRegistry& entities);

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
		void* Add(EntityID entityID, ComponentID id);

		/**
		* [EN]
		* Destroys entityID's component id, if it has one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID がコンポーネント id を持っていれば破棄する。
		*/
		void Remove(EntityID entityID, ComponentID id);

		/**
		* [EN]
		* Destroys every sparse-stored component of entityID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID の全スパース格納コンポーネントを破棄する。
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
		* Returns the sparse set of component id, or nullptr if nothing has
		* added it under its current registration yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コンポーネント id のスパースセットを返す。今の登録でまだ誰も
		* 追加していなければ nullptr。
		*/
		SparseSet* Set(ComponentID id)const;

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
		/// [EN] Where each entity's sparse locations are recorded; owned by the world.
		/// [JP] 各エンティティのスパースの場所を記録する先。ワールドが所有する。
		EntityRegistry& entities_;

		/// [EN] One sparse set per component, indexed by ComponentRecord::index_; nullptr until the component is first added.
		/// [JP] コンポーネントごとのスパースセット。ComponentRecord::index_ で引く。初めて追加されるまでは nullptr。
		DynamicArray<ResourcePtr<SparseSet>> sets_;

		/// [EN] Advanced by every add and remove.
		/// [JP] 追加・削除のたびに進む。
		Uint32 version_ = 0;
	};
}