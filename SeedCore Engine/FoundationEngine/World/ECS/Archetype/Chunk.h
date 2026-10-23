#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct ArchetypeEdge;

	class Archetype;

	/**
	* [EN]
	* Fixed-size (16 KB), cache-line-aligned block of structure-of-arrays
	* component storage for entities sharing a single Archetype's
	* layout. Each component type gets its own contiguous, aligned
	* sub-array within buffer_, sized for capacity_ entities (computed
	* once in the constructor from the archetype's component
	* sizes/alignments). Removal is swap-remove: the moved entity's ID
	* is returned so callers can fix up its location.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単一の Archetype のレイアウトを共有するエンティティ向けの、固定
	* サイズ（16KB）でキャッシュライン整列された、構造体配列（SoA）
	* コンポーネントストレージのブロック。各コンポーネント型は buffer_
	* 内に、capacity_ エンティティ分のサイズを持つ連続した整列済み
	* サブ配列を持つ（コンストラクタでアーキタイプのコンポーネント
	* サイズ/アラインメントから一度だけ計算される）。削除は
	* swap-remove 方式であり、移動したエンティティの ID を返すため、
	* 呼び出し側はその場所を修正できる。
	*/
	class SEEDCORE_API Chunk
	{
	public:
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
		Chunk(Archetype* archetype);

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
		~Chunk();

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
		Uint32 Add(EntityID entityID);

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
		EntityID Remove(Uint32 row);

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
		EntityID Release(Uint32 row);

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
		Uint32 Adopt(Chunk& source, Uint32 sourceRow);

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
		Uint32 Adopt(Chunk& source, Uint32 sourceRow, const ArchetypeEdge& edge);

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
		Uint8* Data(Size layoutIndex);

		/**
		* [EN]
		* Returns the per-element byte size of the component at layoutIndex.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* layoutIndex のコンポーネントの、1要素あたりのバイトサイズを返す。
		*/
		Size Length(Size layoutIndex)const;

		/**
		* [EN]
		* Returns whether the chunk has reached its entity capacity.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* チャンクがエンティティ容量の上限に達しているかどうかを返す。
		*/
		Bool Full()const;

		/**
		* [EN]
		* Returns the maximum number of entities this chunk can hold.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このチャンクが保持できる最大エンティティ数を返す。
		*/
		Size Capacity()const;

		/**
		* [EN]
		* Returns the EntityID stored at row index.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 行 index に格納されている EntityID を返す。
		*/
		EntityID EntityAt(Size index)const;

		/**
		* [EN]
		* Returns the current number of live entities in this chunk.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このチャンク内の現在の有効なエンティティ数を返す。
		*/
		Uint32 EntityCount()const;

	private:
		/// [EN] Total byte size of the raw storage buffer.
		/// [JP] 生ストレージバッファの総バイトサイズ。
		SC_CONSTEX Size chunkSize_ = 16 * 1024;

		/// [EN] Hard cap on the number of entities a single chunk can ever hold.
		/// [JP] 単一チャンクが保持できるエンティティ数の絶対上限。
		SC_CONSTEX Size maxEntitiesPerChunk_ = 1024;

		/// [EN] Hard cap on the number of distinct component types a single archetype/chunk can hold.
		/// [JP] 単一のアーキタイプ/チャンクが保持できる、異なるコンポーネント型数の絶対上限。
		SC_CONSTEX Size maxComponentsPerArchetype_ = 32;

		/// [EN] The archetype this chunk's layout was derived from.
		/// [JP] このチャンクのレイアウトの導出元となったアーキタイプ。
		Archetype* archetype_;

		/// [EN] Per-component-type pointers into buffer_ marking where each type's sub-array begins.
		/// [JP] 各コンポーネント型のサブ配列が buffer_ 内で始まる位置を示す、型ごとのポインタ。
		Uint8* componentDataPointers_[maxComponentsPerArchetype_];

		/// [EN] Per-component-type element byte size, indexed the same as componentDataPointers_.
		/// [JP] 型ごとの1要素あたりのバイトサイズ。componentDataPointers_ と同じインデックスでアクセスする。
		Size componentSizes_[maxComponentsPerArchetype_];

		/// [EN] Number of distinct component types stored in this chunk (mirrors the owning archetype's layout size).
		/// [JP] このチャンクに格納されている、異なるコンポーネント型の数（所有元アーキタイプのレイアウトサイズと一致する）。
		Size componentCount_ = 0;

		/// [EN] Maximum number of entities this chunk can hold, computed from component sizes/alignments and chunkSize_.
		/// [JP] このチャンクが保持できる最大エンティティ数。コンポーネントのサイズ/アラインメントと chunkSize_ から計算される。
		Size capacity_ = 0;

		/// [EN] Raw, cache-line-aligned backing storage for every component's sub-array.
		/// [JP] 各コンポーネントのサブ配列を保持する、キャッシュライン整列された生のバッキングストレージ。
		alignas(64) Uint8 buffer_[chunkSize_];

		/// [EN] Current number of live entities (rows) in this chunk.
		/// [JP] このチャンク内の現在の有効なエンティティ（行）数。
		Uint32 entityCount_ = 0;

		/// [EN] Per-row EntityID, parallel to the component sub-arrays in buffer_.
		/// [JP] buffer_ 内のコンポーネントサブ配列と対応する、行ごとの EntityID。
		alignas(64) EntityID entityIDs_[maxEntitiesPerChunk_];
	};
}
