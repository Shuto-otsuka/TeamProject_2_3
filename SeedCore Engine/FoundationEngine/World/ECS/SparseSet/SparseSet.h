#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Classic sparse-set container mapping EntityID to a T, giving O(1)
	* add/remove/lookup with a densely-packed array for fast iteration.
	* The dense array holds each entity's ID and an owning pointer to its
	* T; the T itself is created in pool_, so it sits together with the
	* other T in pooled memory and never moves while present - a pointer
	* or reference to a stored T (including a coroutine's captured this)
	* stays valid until that entity's T is removed. sparse_ maps an entity
	* ID to its index in the dense array (or UINT32_MAX if absent);
	* removal is swap-remove of the dense entries only, so Remove()
	* returns whichever entity's entry got moved into the vacated slot (so
	* callers can fix up that entity's index cache). Used as the
	* alternative storage strategy to archetype/chunk storage for
	* components registered with ComponentStorage::SparseSet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* EntityID を T へ対応付ける、古典的なスパースセットコンテナ。
	* O(1) の追加・削除・検索を提供しつつ、密配列で高速な走査を可能に
	* する。密配列には各エンティティの ID と、その T を所有するポインタを
	* 持つ。T 本体は pool_ 内に生成されるので、他の T とまとまって
	* プールのメモリ上に置かれ、存在している間は移動しない。そのため
	* 格納された T へのポインタや参照（コルーチンが保持する this も含む）は、
	* そのエンティティの T が削除されるまで有効なまま保たれる。sparse_ は
	* エンティティ ID を密配列内のインデックスへ対応付ける（存在しなければ
	* UINT32_MAX）。削除は密配列のエントリだけを swap-remove する方式で
	* あり、Remove() は空いたスロットへエントリが移動してきたエンティティを
	* 返す（呼び出し側がそのエンティティのインデックスキャッシュを修正
	* できるように）。ComponentStorage::SparseSet で登録された
	* コンポーネント向けの、アーキタイプ/チャンクストレージに代わる
	* 格納戦略として使われる。
	*/
	template<typename T>
	class SparseSet
	{
	private:
		/**
		* [EN]
		* A single dense-array slot: the owning entity's ID alongside the
		* owning pointer to its component data in pool_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 密配列の単一スロット: 所有元エンティティの ID と、pool_ 内に
		* あるそのコンポーネントデータを所有するポインタを保持する。
		*/
		struct Element
		{
			/// [EN] The entity this slot's data belongs to; kept next to the data so Contains() can reject a stale generation.
			/// [JP] このスロットのデータが属するエンティティ。データの隣に置くことで、Contains() が古い世代を弾ける。
			EntityID id_;

			/// [EN] The component data, created in pool_; swap-remove moves only this pointer, never the data it points to.
			/// [JP] pool_ 内に生成されたコンポーネントデータ。swap-remove で動くのはこのポインタだけで、指す先のデータは動かない。
			ResourcePtr<T> data_;
		};

	public:
		/**
		* [EN]
		* Default constructor: starts with empty sparse/dense arrays.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: 空の sparse/dense 配列から開始する。
		*/
		SparseSet() = default;

		/**
		* [EN]
		* Destructor; uses the compiler-generated default. Members are
		* destroyed in reverse declaration order, so dense_ goes first and
		* returns every T to pool_, and pool_ then frees its memory.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デストラクタ。コンパイラ生成のデフォルトを使用する。メンバは宣言と
		* 逆順に破棄されるので、まず dense_ が全ての T を pool_ へ返却し、
		* その後で pool_ が自分のメモリを解放する。
		*/
		~SparseSet() = default;

		/**
		* [EN]
		* Inserts an element for id if not already present, growing sparse_
		* as needed: a default-constructed T is created in pool_ and dense_
		* gets an entry owning it. Returns the entity's dense index. dense_
		* growing only moves the entries, so every existing T keeps its
		* address.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id 用の要素を、まだ存在しなければ挿入する（必要なら sparse_ を
		* 成長させる）: デフォルト構築した T を pool_ 内に生成し、dense_ に
		* それを所有するエントリを追加する。エンティティの密インデックスを
		* 返す。dense_ が伸びて動くのはエントリだけなので、既存の T の
		* アドレスは変わらない。
		*/
		Uint32 Add(EntityID id)
		{
			/// [EN] sparse_ is indexed by the entity's slot index, so it grows to cover the highest index seen; new slots start empty.
			/// [JP] sparse_ はエンティティのスロット番号で引くので、これまでの最大の番号まで伸ばす。新しい枠は空(UINT32_MAX)で始まる。
			if (id.index_ >= sparse_.size())
			{
				sparse_.resize(id.index_ + 1, UINT32_MAX);
			}

			/// [EN] Already present: adding twice is harmless and returns the existing slot.
			/// [JP] 既にある場合は二重に追加せず、今の位置を返す。
			if (sparse_[id.index_] != UINT32_MAX)
			{
				return sparse_[id.index_];
			}

			/// [EN] The new element goes at the end of dense_ with its data created in pool_, and sparse_ points the entity at that position.
			/// [JP] 新しい要素は dense_ の末尾に置き、データは pool_ 内に生成する。sparse_ でエンティティをその位置に結び付ける。
			Uint32 denseIndex = static_cast<Uint32>(dense_.size());
			sparse_[id.index_] = denseIndex;
			dense_.push_back({ id, MakePtr<T>(pool_) });
			return denseIndex;
		}

		/**
		* [EN]
		* Removes id via swap-remove: id's T is destroyed and returned to
		* pool_, and the last dense entry is moved into id's vacated slot
		* (unless id was already last). Only the entry moves - the moved
		* entity's T stays where it is in pool_. Returns the EntityID of
		* whichever entity's entry was moved into the removed slot (so its
		* index cache can be updated), or a default (invalid) EntityID if
		* none was moved (id not found, or it was already last).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id を swap-remove 方式で削除する: id の T を破棄して pool_ へ
		* 返却し、最後の密エントリを id の空いたスロットへ移動する（id が
		* 既に最後だった場合を除く）。動くのはエントリだけで、移動した
		* エンティティの T は pool_ 内の同じ場所に留まる。削除された
		* スロットへエントリが移動したエンティティの EntityID を返す（その
		* インデックスキャッシュを更新できるように）。何も移動されな
		* かった場合（id が見つからない、または既に最後だった場合）は
		* デフォルト（無効）の EntityID を返す。
		*/
		EntityID Remove(EntityID id)
		{
			/// [EN] Contains() also rejects a stale id, so an old handle never removes a newer entity's data.
			/// [JP] Contains() は古い id も弾くので、古いハンドルが新しいエンティティのデータを消すことは無い。
			if (!Contains(id))
			{
				return EntityID{};
			}

			Uint32 removeIndex = sparse_[id.index_];
			Uint32 lastIndex = static_cast<Uint32>(dense_.size()) - 1;

			/// [EN] Removing the last element leaves no gap: dropping it returns its T to pool_, then the entity's sparse_ entry is cleared.
			/// [JP] 最後の要素なら穴は空かないので、取り除いて(その T はここで pool_ へ返却される) sparse_ の枠を空に戻すだけでよい。
			if (removeIndex == lastIndex)
			{
				dense_.pop_back();
				sparse_[id.index_] = UINT32_MAX;
				return EntityID{};
			}

			/// [EN] Swap-remove: move the last entry into the gap (which returns the removed data to pool_) and repoint that entity's sparse_ entry at its new position.
			/// [JP] swap-remove: 最後のエントリを穴へムーブし（消す側のデータはここで pool_ へ返却される）、そのエンティティの sparse_ の枠を新しい位置へ向け直す。
			EntityID lastEntity = dense_[lastIndex].id_;
			dense_[removeIndex] = std::move(dense_[lastIndex]);
			sparse_[lastEntity.index_] = removeIndex;

			/// [EN] The old last slot is now empty; drop it and clear the removed entity's entry.
			/// [JP] 元の末尾は中身が空になったので取り除き、消したエンティティの枠を空に戻す。
			dense_.pop_back();
			sparse_[id.index_] = UINT32_MAX;
			return lastEntity;
		}

		/**
		* [EN]
		* Returns a mutable reference to id's component data (undefined
		* behavior if id is not present).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id のコンポーネントデータへの変更可能な参照を返す（id が
		* 存在しない場合は未定義動作）。
		*/
		T& Get(EntityID id)
		{
			/// [EN] Two lookups, both O(1): entity -> dense index -> data. No presence check, for speed.
			/// [JP] O(1) の参照を2回たどる: エンティティ → 密インデックス → データ。速さのため存在の確認はしない。
			return *dense_[sparse_[id.index_]].data_;
		}

		/**
		* [EN]
		* Const overload of Get().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Get() の const オーバーロード。
		*/
		const T& Get(EntityID id)const
		{
			return *dense_[sparse_[id.index_]].data_;
		}

		/**
		* [EN]
		* Returns whether id currently has an entry in this set. A stale id
		* (its slot index is in range and occupied, but by an entity of a
		* different generation) reports false.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id が現在このセット内にエントリを持つかどうかを返す。古い id
		* （スロットインデックスは範囲内かつ占有済みだが、別の世代の
		* エンティティによる）は false を返す。
		*/
		Bool Contains(EntityID id)const
		{
			/// [EN] In range, occupied, and occupied by this exact entity (the full EntityID comparison includes the generation).
			/// [JP] 範囲内で、枠が埋まっていて、しかもこのエンティティ本人であること(EntityID の比較には世代も含まれる)。
			return id.index_ < sparse_.size()
				&& sparse_[id.index_] != UINT32_MAX
				&& dense_[sparse_[id.index_]].id_ == id;
		}

		/**
		* [EN]
		* Returns id's index within the dense array (undefined behavior
		* if id is not present).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 密配列内における id のインデックスを返す（id が存在しない
		* 場合は未定義動作）。
		*/
		Uint32 IndexOf(EntityID id)const
		{
			return sparse_[id.index_];
		}

		/**
		* [EN]
		* Returns the underlying dense array, for direct iteration.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直接走査するための、内部の密配列を返す。
		*/
		const DynamicArray<Element>& Dense()const
		{
			/// [EN] Order is not stable: swap-remove moves the last entry whenever something is removed. Each entry's data_ still points at the same T in pool_.
			/// [JP] 並び順は保たれない。何かを消すたびに、swap-remove で最後のエントリが移動する。各エントリの data_ が指す pool_ 内の T は変わらない。
			return dense_;
		}

		/**
		* [EN]
		* Returns the current number of entities stored in this set.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このセットに現在格納されているエンティティ数を返す。
		*/
		Uint32 Length()const
		{
			return static_cast<Uint32>(dense_.size());
		}

	private:
		/// [EN] Maps an entity's slot index to its index in dense_, or UINT32_MAX if absent. Sized by the highest slot index, so it can be sparse.
		/// [JP] エンティティのスロット番号を dense_ 内の位置へ対応付ける。無ければ UINT32_MAX。最大のスロット番号まで伸びるので、まばらになりうる。
		DynamicArray<Uint32> sparse_;

		/// [EN] Where every T lives. A single shard (LogSize 0), so successive T are carved from the same backing memory side by side instead of being spread round-robin over many shards.
		///      PackedSynchronizedPointer keeps the freelist head in one word and is defined entirely in the header, so a module that instantiates this template (e.g. UserProject for its scripts) needs nothing exported from FoundationEngine.
		///      Declared before dense_ so it is destroyed after dense_, outliving every pointer dense_ holds.
		/// [JP] 全ての T の置き場所。シャードは1つ(LogSize 0)なので、続けて生成した T は複数のシャードへ順番に振り分けられず、同じバッキングメモリから隣り合って切り出される。
		///      PackedSynchronizedPointer はフリーリストの先頭を1ワードに収め、ヘッダだけで定義が完結するので、このテンプレートを実体化するモジュール(例: スクリプトを持つ UserProject)は FoundationEngine からのエクスポートを必要としない。
		///      dense_ より前に宣言し、dense_ より後に破棄されるようにして、dense_ が持つ全ポインタより長く生きるようにする。
		ObjectPool<T, PackedSynchronizedPointer<>, 0> pool_;

		/// [EN] Densely-packed storage of every present entity's ID and owning pointer to its component data.
		/// [JP] 現在存在する全エンティティの ID と、そのコンポーネントデータを所有するポインタを、密に詰めて格納するストレージ。
		DynamicArray<Element> dense_;
	};

	/**
	* [EN]
	* Type-erased interface over a SparseSet<T>, letting World/Actor
	* manipulate sparse-set-backed components without knowing their
	* concrete type at the call site (the concrete type is captured by
	* SparseSetStorage<T> and constructed via ComponentMetadata::createSparseStorage_).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SparseSet<T> に対する型消去されたインターフェース。World/Actor が、
	* 呼び出し側で具体的な型を知らずとも、スパースセットで裏付けられた
	* コンポーネントを操作できるようにする（具体的な型は
	* SparseSetStorage<T> に捕捉され、ComponentMetadata::createSparseStorage_
	* 経由で構築される）。
	*/
	class InterfaceSparseSetStorage
	{
	public:
		/**
		* [EN]
		* Virtual destructor; uses the compiler-generated default.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想デストラクタ。コンパイラ生成のデフォルトを使用する。
		*/
		virtual ~InterfaceSparseSetStorage() = default;

		/**
		* [EN]
		* Type-erased counterpart of SparseSet<T>::Remove: removes the
		* entity's entry, if present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SparseSet<T>::Remove の型消去版: エンティティのエントリが
		* 存在すれば削除する。
		*/
		virtual void Remove(EntityID id) = 0;

		/**
		* [EN]
		* Type-erased counterpart of SparseSet<T>::Add: inserts a
		* default-constructed element for the entity if not present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SparseSet<T>::Add の型消去版: エンティティが未登録なら
		* デフォルト構築した要素を挿入する。
		*/
		virtual void Add(EntityID id) = 0;

		/**
		* [EN]
		* Type-erased counterpart of SparseSet<T>::Contains.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SparseSet<T>::Contains の型消去版。
		*/
		virtual Bool Contains(EntityID id)const = 0;

		/**
		* [EN]
		* Returns a type-erased pointer to the entity's component data,
		* or nullptr if the entity has no entry.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンティティのコンポーネントデータへの型消去されたポインタを
		* 返す。エンティティにエントリが無ければ nullptr を返す。
		*/
		virtual void* Raw(EntityID id) = 0;
	};

	/**
	* [EN]
	* Concrete InterfaceSparseSetStorage implementation wrapping a
	* SparseSet<T>, forwarding every virtual call to it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SparseSet<T> を包む、具体的な InterfaceSparseSetStorage の実装。
	* すべての仮想呼び出しをそれへ転送する。
	*/
	template<typename T>
	class SparseSetStorage final :public InterfaceSparseSetStorage
	{
	private:
		/// [EN] World's typed accessors cast down to SparseSetStorage<T> and read data_ directly.
		/// [JP] World の型付きアクセスは SparseSetStorage<T> へキャストして、data_ を直接読む。
		friend class World;

	public:
		/**
		* [EN]
		* Forwards to data_.Remove(id).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data_.Remove(id) へ転送する。
		*/
		void Remove(EntityID id) override
		{
			data_.Remove(id);
		}

		/**
		* [EN]
		* Forwards to data_.Add(id).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data_.Add(id) へ転送する。
		*/
		void Add(EntityID id) override
		{
			data_.Add(id);
		}

		/**
		* [EN]
		* Forwards to data_.Contains(id).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data_.Contains(id) へ転送する。
		*/
		Bool Contains(EntityID id)const override
		{
			return data_.Contains(id);
		}

		/**
		* [EN]
		* Returns a pointer to data_.Get(id), or nullptr if id is not present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data_.Get(id) へのポインタを返す。id が存在しなければ nullptr
		* を返す。
		*/
		void* Raw(EntityID id) override
		{
			/// [EN] Get() does not check presence, so it is guarded here to turn a miss into nullptr.
			/// [JP] Get() は存在を確かめないので、ここで確認して、無い場合は nullptr を返す。
			if (!data_.Contains(id))
			{
				return nullptr;
			}

			return &data_.Get(id);
		}

	private:
		/// [EN] The underlying typed sparse set this storage wraps.
		/// [JP] このストレージが包む、内部の型付きスパースセット。
		SparseSet<T> data_;
	};
}
