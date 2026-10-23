#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	class Archetype;
	class Chunk;

	/**
	* [EN]
	* The cached move from one archetype to the archetype with one
	* component added or removed: the target, and how each of the target's
	* columns is filled from the source's columns.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ある Archetype から、コンポーネントを1つ足した/引いた Archetype への
	* 移動をキャッシュしたもの。移動先と、移動先の各列を移動元のどの列から
	* 埋めるか。
	*/
	struct ArchetypeEdge
	{
		/// [EN] The archetype the entity moves to.
		/// [JP] エンティティの移動先のアーキタイプ。
		Archetype* target_ = nullptr;

		/// [EN] For each column of target_, the source column it is relocated from, or UINT32_MAX for the added component's column, which is constructed.
		/// [JP] target_ の各列について、移設元になる移動元の列。足したコンポーネントの列は UINT32_MAX で、そこは構築する。
		DynamicArray<Uint32> sourceColumns_;

		/// [EN] The source column of the removed component, destructed on the move; UINT32_MAX for an edge that adds a component.
		/// [JP] 引いたコンポーネントの移動元の列。移動のときに破棄する。コンポーネントを足す edge では UINT32_MAX。
		Uint32 removedColumn_ = UINT32_MAX;
	};

	/**
	* [EN]
	* Represents a unique combination of component types (a "layout")
	* shared by every entity routed to it. Owns a pool of Chunk objects
	* that actually store the component data, a Bitset signature used to
	* quickly test whether this archetype has a given component, and the
	* edges to the archetypes one component away.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* それにルーティングされる全エンティティが共有する、コンポーネント
	* 型の一意な組み合わせ（「レイアウト」）を表す。実際にコンポーネント
	* データを格納する Chunk オブジェクトのプール、このアーキタイプが
	* 特定のコンポーネントを持つかを高速に判定するための Bitset
	* シグネチャ、コンポーネント1つ分だけ違うアーキタイプへの edge を
	* 所有する。
	*/
	class SEEDCORE_API Archetype
	{
	public:
		/**
		* [EN]
		* Constructs archetype number index for the given component layout,
		* building its signature bitset from each component's registered ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定されたコンポーネントレイアウト用の、番号 index のアーキタイプを
		* 構築し、各コンポーネントの登録済み ID からシグネチャビットセットを
		* 構築する。
		*/
		Archetype(const DynamicArray<ComponentID>& layout, Uint32 index);

		/**
		* [EN]
		* Destructor; uses the compiler-generated default.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デストラクタ。コンパイラ生成のデフォルトを使用する。
		*/
		~Archetype() = default;

		/**
		* [EN]
		* Returns the ordered list of component types making up this archetype.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このアーキタイプを構成する、順序付きのコンポーネント型一覧を
		* 返す。
		*/
		const DynamicArray<ComponentID>& Layout()const;

		/**
		* [EN]
		* Returns this archetype's number, assigned in creation order across
		* the process; storages index their per-archetype data by it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロセス全体で作られた順に振られる、このアーキタイプの番号を返す。
		* ストレージはアーキタイプごとのデータをこれで引く。
		*/
		Uint32 Index()const;

		/**
		* [EN]
		* Returns the bitset signature identifying which components this
		* archetype has.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このアーキタイプがどのコンポーネントを持つかを識別する、
		* ビットセットシグネチャを返す。
		*/
		const Bitset& Signature()const;

		/**
		* [EN]
		* Returns the edge to this archetype with component id added,
		* computing and caching it the first time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このアーキタイプにコンポーネント id を足したアーキタイプへの edge を
		* 返す。初回だけ計算してキャッシュする。
		*/
		const ArchetypeEdge& With(ComponentID id);

		/**
		* [EN]
		* Returns the edge to this archetype with component id removed,
		* computing and caching it the first time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このアーキタイプからコンポーネント id を引いたアーキタイプへの edge を
		* 返す。初回だけ計算してキャッシュする。
		*/
		const ArchetypeEdge& Without(ComponentID id);

		/**
		* [EN]
		* Allocates (or recycles from the pool) a new Chunk for this archetype.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このアーキタイプ用の新しい Chunk を確保する（またはプールから
		* 再利用する）。
		*/
		Chunk* CreateChunk();

		/**
		* [EN]
		* Returns chunk to the pool for later reuse.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* chunk を後で再利用できるようプールへ返却する。
		*/
		void RecycleChunk(Chunk* chunk);

	private:
		/// [EN] The ordered list of component types making up this archetype.
		/// [JP] このアーキタイプを構成する、順序付きのコンポーネント型一覧。
		DynamicArray<ComponentID> layout_;

		/// [EN] This archetype's number, assigned in creation order across the process.
		/// [JP] プロセス全体で作られた順に振られる、このアーキタイプの番号。
		Uint32 index_ = 0;

		/// [EN] Bitset signature identifying which components this archetype has, indexed by each component's registered ID.
		/// [JP] このアーキタイプがどのコンポーネントを持つかを識別するビットセットシグネチャ。各コンポーネントの登録済み ID でインデックスされる。
		Bitset signature_;

		/// [EN] Cached edges to the archetypes with one component added, indexed by that component's ComponentRecord::index_; nullptr until first used.
		/// [JP] コンポーネントを1つ足したアーキタイプへのキャッシュ済み edge。足したコンポーネントの ComponentRecord::index_ で引く。初めて使うまでは nullptr。
		DynamicArray<ResourcePtr<ArchetypeEdge>> withEdges_;

		/// [EN] Cached edges to the archetypes with one component removed, indexed by that component's ComponentRecord::index_; nullptr until first used.
		/// [JP] コンポーネントを1つ引いたアーキタイプへのキャッシュ済み edge。引いたコンポーネントの ComponentRecord::index_ で引く。初めて使うまでは nullptr。
		DynamicArray<ResourcePtr<ArchetypeEdge>> withoutEdges_;

		/// [EN] Pool from which this archetype's Chunk objects are allocated/recycled.
		/// [JP] このアーキタイプの Chunk オブジェクトを確保・再利用するためのプール。
		ObjectPool<Chunk> chunkPool_;
	};
}
