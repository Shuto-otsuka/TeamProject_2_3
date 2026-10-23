#include <FoundationEngine/World/ECS/SparseSet/SparseSet.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
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
	SparseSet::SparseSet(ComponentID id) :id_(id), epoch_(id->epoch_)
	{
		elementSize_ = ComponentRegistry::ComponentSize(id_);
		elementAlignment_ = ComponentRegistry::ComponentAlignment(id_);

		/// [EN] sizeof is never 0, so the division is safe; a component larger than a page still gets one per page.
		/// [JP] sizeof は 0 にならないので割り算は安全。1ページより大きいコンポーネントでも 1ページに1つは入れる。
		pageCapacity_ = Max<Size>(1, pageSize_ / elementSize_);
	}

	/**
	* [EN]
	* Destructs every component still present and frees every page.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* まだ存在する全コンポーネントを破棄し、全ページを解放する。
	*/
	SparseSet::~SparseSet()
	{
		/// [EN] An emptied set may outlive its component's registration, so the metadata is read only when something is left to destroy.
		/// [JP] 空になったセットはコンポーネントの登録より長く残りうるので、破棄するものが残っているときだけ metadata を読む。
		if (!data_.empty())
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(id_);
			for (void* data : data_)
			{
				meta.destruct_(data);
			}
		}

		for (Uint8* page : pages_)
		{
			::operator delete(page, std::align_val_t(elementAlignment_));
		}
	}

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
	Uint32 SparseSet::Add(EntityID entityID)
	{
		void* slot = nullptr;
		if (!freeSlots_.empty())
		{
			slot = freeSlots_.back();
			freeSlots_.pop_back();
		}
		else
		{
			/// [EN] The last page is full (or there is none yet), so a new page is allocated; existing pages never move.
			/// [JP] 最後のページが満杯（またはまだ無い）なので新しいページを確保する。既存のページは動かない。
			if (pages_.empty() || lastPageUsed_ == pageCapacity_)
			{
				Uint8* page = static_cast<Uint8*>(::operator new(pageCapacity_ * elementSize_, std::align_val_t(elementAlignment_)));
				pages_.push_back(page);
				lastPageUsed_ = 0;
			}

			slot = pages_.back() + (elementSize_ * lastPageUsed_);
			++lastPageUsed_;
		}

		ComponentRegistry::Get(id_).construct_(slot);

		Uint32 row = static_cast<Uint32>(entityIDs_.size());
		entityIDs_.push_back(entityID);
		data_.push_back(slot);
		return row;
	}

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
	EntityID SparseSet::Remove(Uint32 row)
	{
		if (row >= entityIDs_.size())
		{
			return EntityID{};
		}

		ComponentRegistry::Get(id_).destruct_(data_[row]);
		freeSlots_.push_back(data_[row]);

		Uint32 lastRow = static_cast<Uint32>(entityIDs_.size()) - 1;
		if (row == lastRow)
		{
			entityIDs_.pop_back();
			data_.pop_back();
			return EntityID{};
		}

		/// [EN] Only the row moves; the moved entity's component stays at the same address, so just its row needs updating.
		/// [JP] 動くのは行だけ。移動したエンティティのコンポーネントは同じアドレスのままなので、直すのは行だけでよい。
		EntityID movedEntityID = entityIDs_[lastRow];
		entityIDs_[row] = movedEntityID;
		data_[row] = data_[lastRow];

		entityIDs_.pop_back();
		data_.pop_back();
		return movedEntityID;
	}

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
	Uint32 SparseSet::Epoch()const
	{
		return epoch_;
	}

	/**
	* [EN]
	* Returns the component at row; its address never changes while present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* row のコンポーネントを返す。存在している間、そのアドレスは変わらない。
	*/
	void* SparseSet::Data(Uint32 row)const
	{
		return data_[row];
	}

	/**
	* [EN]
	* Returns the entity at row.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* row のエンティティを返す。
	*/
	EntityID SparseSet::EntityAt(Uint32 row)const
	{
		return entityIDs_[row];
	}

	/**
	* [EN]
	* Returns the number of entities currently in this set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このセットに現在いるエンティティ数を返す。
	*/
	Uint32 SparseSet::EntityCount()const
	{
		return static_cast<Uint32>(entityIDs_.size());
	}
}