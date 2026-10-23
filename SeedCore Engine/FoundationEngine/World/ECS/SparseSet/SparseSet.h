#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	/**
	* [EN]
	* Storage for every instance of one sparse-stored component, the
	* counterpart of Chunk. Each row pairs an entity with a pointer to its
	* component, which is constructed in fixed-size pages and never moves
	* while present, so a pointer to it (including a coroutine's captured
	* this) stays valid until it is removed. Removal is swap-remove of the
	* rows only; the moved entity is returned so its location's row can be
	* updated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スパース格納の1種類のコンポーネントの全インスタンスを持つ格納先で、
	* Chunk と対になる。各行はエンティティと、そのコンポーネントへの
	* ポインタの組。コンポーネントは固定サイズのページ内に構築され、
	* 存在している間は移動しないので、それへのポインタ（コルーチンが
	* 保持する this も含む）は削除されるまで有効なまま保たれる。削除は
	* 行だけの swap-remove で、移動したエンティティを返し、その場所の行を
	* 直せるようにする。
	*/
	class SEEDCORE_API SparseSet :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Constructs an empty set for the component id, reading its size and
		* alignment from the registry.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コンポーネント id 用の空のセットを構築する。サイズとアラインメントは
		* レジストリから読む。
		*/
		SparseSet(ComponentID id);

		/**
		* [EN]
		* Destructs every component still present and frees every page.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* まだ存在する全コンポーネントを破棄し、全ページを解放する。
		*/
		~SparseSet();

		/**
		* [EN]
		* Default-constructs a component for entityID in a free slot (reusing a
		* removed one first) and appends a row for it. Returns the new row.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空き枠（削除済みの枠を優先して再利用）に entityID 用のコンポーネントを
		* デフォルト構築し、その行を末尾に追加する。新しい行を返す。
		*/
		Uint32 Add(EntityID entityID);

		/**
		* [EN]
		* Destructs the component at row and frees its slot, then moves the
		* last row into the gap. Returns the entity whose row changed, or a
		* default (invalid) EntityID if row was the last one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* row のコンポーネントを破棄して枠を空け、最後の行を空いた位置へ
		* 移す。行が変わったエンティティを返す。row が最後の行だった場合は
		* デフォルト（無効）の EntityID を返す。
		*/
		EntityID Remove(Uint32 row);

		/**
		* [EN]
		* Returns the component at row; its address never changes while present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* row のコンポーネントを返す。存在している間、そのアドレスは変わらない。
		*/
		void* Data(Uint32 row)const;

		/**
		* [EN]
		* Returns the epoch of the component's record when this set was
		* created; a different current epoch means the component has been
		* unregistered since, so this set belongs to the old registration.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このセットを作ったときのコンポーネントの Record の epoch を返す。
		* 今の epoch と違えばその後に登録解除されたということで、このセットは
		* 古い登録のもの。
		*/
		Uint32 Epoch()const;

		/**
		* [EN]
		* Returns the entity at row.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* row のエンティティを返す。
		*/
		EntityID EntityAt(Uint32 row)const;

		/**
		* [EN]
		* Returns the number of entities currently in this set.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このセットに現在いるエンティティ数を返す。
		*/
		Uint32 EntityCount()const;

	private:
		/// [EN] Target byte size of one page, matching a chunk.
		/// [JP] 1ページの目安のバイトサイズ。チャンクと同じ。
		SC_CONSTEX Size pageSize_ = 16 * 1024;

		/// [EN] The component this set stores.
		/// [JP] このセットが格納するコンポーネント。
		ComponentID id_ = nullptr;

		/// [EN] The record's epoch when this set was created.
		/// [JP] このセットを作ったときの Record の epoch。
		Uint32 epoch_ = 0;

		/// [EN] Byte size of one component; already a multiple of its alignment, so it is also the stride inside a page.
		/// [JP] コンポーネント1つのバイトサイズ。アラインメントの倍数なので、そのままページ内の間隔になる。
		Size elementSize_ = 0;

		/// [EN] Alignment every page is allocated with.
		/// [JP] 各ページを確保するときのアラインメント。
		Size elementAlignment_ = 0;

		/// [EN] Number of components one page holds; at least 1 even for a component larger than pageSize_.
		/// [JP] 1ページに入るコンポーネント数。pageSize_ より大きいコンポーネントでも最低 1。
		Size pageCapacity_ = 0;

		/// [EN] Every page allocated so far; pages are only freed in the destructor, so slots never move.
		/// [JP] これまでに確保した全ページ。ページはデストラクタでしか解放しないので、枠は動かない。
		DynamicArray<Uint8*> pages_;

		/// [EN] Number of slots handed out from the last page.
		/// [JP] 最後のページから払い出した枠の数。
		Size lastPageUsed_ = 0;

		/// [EN] Slots freed by Remove, reused by Add before a new slot is carved from a page.
		/// [JP] Remove で空いた枠。ページから新しい枠を切り出す前に Add が再利用する。
		DynamicArray<void*> freeSlots_;

		/// [EN] Entity of each row.
		/// [JP] 各行のエンティティ。
		DynamicArray<EntityID> entityIDs_;

		/// [EN] Component of each row, parallel to entityIDs_.
		/// [JP] 各行のコンポーネント。entityIDs_ と並行。
		DynamicArray<void*> data_;
	};
}