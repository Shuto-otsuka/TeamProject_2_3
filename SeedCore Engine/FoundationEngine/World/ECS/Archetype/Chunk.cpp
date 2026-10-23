#include <FoundationEngine/World/ECS/Archetype/Chunk.h>
#include <FoundationEngine/World/ECS/Archetype/Archetype.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a chunk for archetype, computing capacity_ and each
	* component sub-array's offset/size from the archetype's layout.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* archetype 用のチャンクを構築する。アーキタイプのレイアウトから
	* capacity_ と各コンポーネントサブ配列のオフセット/サイズを計算する。
	*/
	Chunk::Chunk(Archetype* archetype) : archetype_(archetype)
	{
		const auto& layout = archetype_->Layout();
		componentCount_ = layout.size();

		/// [EN] Per-entity byte size of one row, and the worst-case padding every sub-array may need to reach its alignment.
		/// [JP] 1行(エンティティ1体)分のバイト数と、各サブ配列が整列のために必要としうる最悪のパディング量。
		Size totalSize = 0;
		Size alignmentSlack = 0;
		for (Size index = 0; index < componentCount_; ++index)
		{
			ComponentID id = layout[index];
			totalSize += ComponentRegistry::ComponentSize(id);

			/// [EN] Aligning a sub-array start wastes at most alignment - 1 bytes; a zero alignment (no data) wastes nothing.
			/// [JP] サブ配列の先頭を揃えるときに無駄になるのは最大で alignment - 1 バイト。alignment が 0(データ無し)なら無駄は無い。
			Size alignment = ComponentRegistry::ComponentAlignment(id);
			alignmentSlack += (alignment > 0) ? (alignment - 1) : 0;
		}

		if (totalSize == 0)
		{
			/// [EN] No sized components (e.g. tag-only archetype): capacity is bounded only by maxEntitiesPerChunk_.
			/// [JP] サイズを持つコンポーネントが無い場合（タグのみのアーキタイプなど）: 容量は maxEntitiesPerChunk_ のみで制限される。
			capacity_ = maxEntitiesPerChunk_;
		}
		else
		{
			/// [EN] Reserve worst-case alignment padding up front, then divide the remaining usable bytes by the per-entity component size.
			/// [JP] 最悪ケースのアラインメント用パディングをあらかじめ確保し、残りの使用可能バイト数をエンティティ1体あたりのコンポーネントサイズで割る。
			Size usable = (chunkSize_ > alignmentSlack) ? (chunkSize_ - alignmentSlack) : 0;
			capacity_ = usable / totalSize;

			/// [EN] Small rows could fit more entities than entityIDs_ has slots, so the capacity is capped there.
			/// [JP] 1行が小さいと entityIDs_ の枠より多く入ってしまうので、容量はそこで頭打ちにする。
			if (capacity_ > maxEntitiesPerChunk_)
			{
				capacity_ = maxEntitiesPerChunk_;
			}
		}

		/// [EN] Carve buffer_ into one aligned sub-array per component, each capacity_ elements long, in layout order.
		/// [JP] buffer_ をレイアウトの順に、コンポーネントごとの整列済みサブ配列(それぞれ capacity_ 要素分)へ切り分ける。
		Uint8* cursor = buffer_;
		for (Size index = 0; index < componentCount_; ++index)
		{
			ComponentID id = layout[index];
			Size size = ComponentRegistry::ComponentSize(id);
			Size alignment = ComponentRegistry::ComponentAlignment(id);

			/// [EN] Advance cursor to the next multiple of alignment: (alignment - address % alignment) % alignment is 0 when already aligned.
			/// [JP] cursor を alignment の次の倍数まで進める。(alignment - アドレス % alignment) % alignment は、揃っていれば 0 になる。
			Size padding = (alignment - (reinterpret_cast<std::uintptr_t>(cursor) % alignment)) % alignment;
			cursor += padding;

			componentSizes_[index] = size;
			componentDataPointers_[index] = cursor;

			/// [EN] The next sub-array starts right after this one's capacity_ elements.
			/// [JP] 次のサブ配列は、このサブ配列の capacity_ 要素分の直後から始まる。
			cursor += (size * capacity_);
		}
	}

	/**
	* [EN]
	* Destructs every live component instance currently stored in this chunk.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このチャンクに現在格納されている、全ての有効なコンポーネント
	* インスタンスを破棄する。
	*/
	Chunk::~Chunk()
	{
		/// [EN] buffer_ is raw bytes, so each live component must be destructed explicitly through its registered destructor.
		/// [JP] buffer_ は生のバイト列なので、有効な各コンポーネントは登録済みのデストラクタで明示的に破棄する。
		for (Uint32 entityIndex = 0; entityIndex < entityCount_; ++entityIndex)
		{
			for (Size componentIndex = 0; componentIndex < componentCount_; ++componentIndex)
			{
				/// [EN] Element entityIndex of a sub-array sits at its start plus entityIndex elements.
				/// [JP] サブ配列の entityIndex 番目の要素は、先頭から entityIndex 要素分進んだ位置にある。
				const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[componentIndex]);
				Uint8* slot = componentDataPointers_[componentIndex] + (componentSizes_[componentIndex] * entityIndex);
				meta.destruct_(slot);
			}
		}
	}

	/**
	* [EN]
	* Appends a row for entityID, default-constructing each component,
	* and returns the new row. The chunk must not be full.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID 用の行を末尾に追加して各コンポーネントをデフォルト構築し、
	* 新しい行を返す。チャンクは満杯であってはならない。
	*/
	Uint32 Chunk::Add(EntityID entityID)
	{
		SC_ASSERT(entityCount_ < capacity_, "満杯のチャンクに Add されました。呼び出し側で Full() を確認してチャンクを用意してください。");

		/// [EN] The new row goes at the end: record its entity, then default-construct each component in place.
		/// [JP] 新しい行は末尾に置く。エンティティを記録してから、各コンポーネントをその場でデフォルト構築する。
		Uint32 row = entityCount_;
		entityIDs_[row] = entityID;

		for (Size index = 0;index < componentCount_;++index)
		{
			Uint8* slot = componentDataPointers_[index] + (componentSizes_[index] * row);
			const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[index]);
			meta.construct_(slot);
		}

		++entityCount_;
		return row;
	}

	/**
	* [EN]
	* Destructs every component at row, then releases the row. Returns the
	* entity whose row changed, or a default (invalid) EntityID if row was
	* the last one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* row の全コンポーネントを破棄してから、その行を解放する。行が
	* 変わったエンティティを返す。row が最後の行だった場合はデフォルト
	* （無効）の EntityID を返す。
	*/
	EntityID Chunk::Remove(Uint32 row)
	{
		if (row >= entityCount_)
		{
			return EntityID{};
		}

		for (Size index = 0;index < componentCount_;++index)
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[index]);
			Uint8* slot = componentDataPointers_[index] + (componentSizes_[index] * row);
			meta.destruct_(slot);
		}

		return Release(row);
	}

	/**
	* [EN]
	* Fills row, whose components are already gone, by relocating the last
	* row into it. Returns the entity whose row changed, or a default
	* (invalid) EntityID if row was the last one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネントが既に無い row を、最後の行を移設して埋める。行が
	* 変わったエンティティを返す。row が最後の行だった場合はデフォルト
	* （無効）の EntityID を返す。
	*/
	EntityID Chunk::Release(Uint32 row)
	{
		if (row >= entityCount_)
		{
			return EntityID{};
		}

		/// [EN] Releasing the last row leaves no gap, so nothing moves.
		/// [JP] 最後の行を解放した場合は穴が空かないので、何も移動しない。
		Uint32 lastRow = entityCount_ - 1;
		if (row == lastRow)
		{
			--entityCount_;
			return EntityID{};
		}

		/// [EN] relocate_ leaves the last row empty, so it can simply fall past the new end.
		/// [JP] relocate_ は最後の行を空にするので、そのまま新しい末尾の外に出してよい。
		for (Size index = 0;index < componentCount_;++index)
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[index]);
			Uint8* dest = componentDataPointers_[index] + (componentSizes_[index] * row);
			Uint8* source = componentDataPointers_[index] + (componentSizes_[index] * lastRow);
			meta.relocate_(dest, source);
		}

		EntityID movedEntityID = entityIDs_[lastRow];
		entityIDs_[row] = movedEntityID;
		--entityCount_;
		return movedEntityID;
	}

	/**
	* [EN]
	* Takes over row sourceRow of source, a chunk of the same archetype, as
	* a new row of this chunk by relocating every column, leaving source's
	* row empty for Release. Returns the new row. The chunk must not be
	* full.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 同じアーキタイプのチャンク source の sourceRow 行目を、全列を移設して
	* このチャンクの新しい行として引き取る。source の行は空になり Release
	* できる状態になる。新しい行を返す。チャンクは満杯であってはならない。
	*/
	Uint32 Chunk::Adopt(Chunk& source, Uint32 sourceRow)
	{
		SC_ASSERT(entityCount_ < capacity_, "満杯のチャンクに Adopt されました。呼び出し側で Full() を確認して新しいチャンクを用意してください。");
		SC_ASSERT(source.archetype_ == archetype_, "edge 無しの Adopt は同じアーキタイプのチャンク同士でしか使えません。");

		Uint32 row = entityCount_;
		entityIDs_[row] = source.entityIDs_[sourceRow];

		/// [EN] Both chunks share one layout, so every column maps to the same column.
		/// [JP] 両方のチャンクは同じレイアウトなので、各列は同じ列に対応する。
		for (Size index = 0;index < componentCount_;++index)
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[index]);
			Uint8* dest = componentDataPointers_[index] + (componentSizes_[index] * row);
			Uint8* sourceSlot = source.componentDataPointers_[index] + (source.componentSizes_[index] * sourceRow);
			meta.relocate_(dest, sourceSlot);
		}

		++entityCount_;
		return row;
	}

	/**
	* [EN]
	* Takes over row sourceRow of source as a new row of this chunk, using
	* edge's column mapping: shared components are relocated, components
	* only here are constructed, and components only in source are
	* destructed, leaving source's row empty for Release. Returns the new
	* row. The chunk must not be full.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* edge の列の対応を使って、source の sourceRow 行目をこのチャンクの新しい
	* 行として引き取る。共通のコンポーネントは移設し、ここにしか無いものは
	* 構築し、source にしか無いものは破棄するので、source の行は空になり
	* Release できる状態になる。新しい行を返す。チャンクは満杯であっては
	* ならない。
	*/
	Uint32 Chunk::Adopt(Chunk& source, Uint32 sourceRow, const ArchetypeEdge& edge)
	{
		SC_ASSERT(entityCount_ < capacity_, "満杯のチャンクに Adopt されました。呼び出し側で Full() を確認して新しいチャンクを用意してください。");

		Uint32 row = entityCount_;
		entityIDs_[row] = source.entityIDs_[sourceRow];

		/// [EN] Every column here either comes from a source column, or is new and is constructed.
		/// [JP] ここの各列は、source のどれかの列から来るか、新しく構築するかのどちらか。
		for (Size index = 0;index < componentCount_;++index)
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[index]);
			Uint8* dest = componentDataPointers_[index] + (componentSizes_[index] * row);
			Uint32 sourceColumn = edge.sourceColumns_[index];
			if (sourceColumn == UINT32_MAX)
			{
				meta.construct_(dest);
			}
			else
			{
				Uint8* sourceSlot = source.componentDataPointers_[sourceColumn] + (source.componentSizes_[sourceColumn] * sourceRow);
				meta.relocate_(dest, sourceSlot);
			}
		}

		/// [EN] The removed component has no place here, so it is destroyed and the whole source row ends up empty.
		/// [JP] 引いたコンポーネントはここに行き先が無いので破棄し、source の行は全て空になる。
		if (edge.removedColumn_ != UINT32_MAX)
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(source.archetype_->Layout()[edge.removedColumn_]);
			Uint8* sourceSlot = source.componentDataPointers_[edge.removedColumn_] + (source.componentSizes_[edge.removedColumn_] * sourceRow);
			meta.destruct_(sourceSlot);
		}

		++entityCount_;
		return row;
	}

	/**
	* [EN]
	* Returns a pointer to the start of the component sub-array at
	* layoutIndex within the archetype's layout.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アーキタイプのレイアウト内における layoutIndex 番目の
	* コンポーネントサブ配列の先頭へのポインタを返す。
	*/
	Uint8* Chunk::Data(Size layoutIndex)
	{
		return componentDataPointers_[layoutIndex];
	}

	/**
	* [EN]
	* Returns the per-element byte size of the component at layoutIndex.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* layoutIndex のコンポーネントの、1要素あたりのバイトサイズを返す。
	*/
	Size Chunk::Length(Size layoutIndex)const
	{
		return componentSizes_[layoutIndex];
	}

	/**
	* [EN]
	* Returns whether the chunk has reached its entity capacity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* チャンクがエンティティ容量の上限に達しているかどうかを返す。
	*/
	Bool Chunk::Full()const
	{
		return entityCount_ >= capacity_;
	}

	/**
	* [EN]
	* Returns the maximum number of entities this chunk can hold.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このチャンクが保持できる最大エンティティ数を返す。
	*/
	Size Chunk::Capacity()const
	{
		return capacity_;
	}

	/**
	* [EN]
	* Returns the EntityID stored at row index.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 行 index に格納されている EntityID を返す。
	*/
	EntityID Chunk::EntityAt(Size index)const
	{
		return entityIDs_[index];
	}

	/**
	* [EN]
	* Returns the current number of live entities in this chunk.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このチャンク内の現在の有効なエンティティ数を返す。
	*/
	Uint32 Chunk::EntityCount()const
	{
		return entityCount_;
	}
}
