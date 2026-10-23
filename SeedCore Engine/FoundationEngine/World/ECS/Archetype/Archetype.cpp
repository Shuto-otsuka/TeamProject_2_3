#include <FoundationEngine/World/ECS/Archetype/Archetype.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/World/ECS/Archetype/Chunk.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs an archetype for the given component layout, building
	* its signature bitset from each component's registered ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定されたコンポーネントレイアウト用のアーキタイプを構築し、各
	* コンポーネントの登録済み ID からシグネチャビットセットを構築する。
	*/
	Archetype::Archetype(const DynamicArray<ComponentID>& layout, Uint32 index) : layout_(layout), index_(index)
	{
		/// [EN] One bit per component type, indexed by its dense internal ID, so a query can match archetypes with a bitset test.
		/// [JP] コンポーネント型ごとに、その密な内部 ID の位置へビットを1つ立てる。クエリはビットセットの判定でアーキタイプを照合できる。
		for (ComponentID id : layout)
		{
			/// [EN] Grow the signature bitset lazily to fit each component's dense internal ID before setting its bit.
			/// [JP] 各コンポーネントの密な内部 ID にビットを立てる前に、それを収められるようシグネチャビットセットを遅延的に拡張する。
			Size internalID = ComponentRegistry::GetID(id);

			/// [EN] Dense IDs are handed out in first-use order, so a later-registered type may need more bits than the set has.
			/// [JP] 密な ID は最初に使われた順に配られるので、後から登録された型ではビットセットの長さが足りないことがある。
			if (internalID >= signature_.size())
			{
				signature_.resize(internalID + 1);
			}

			signature_.set(internalID);
		}
	}

	/**
	* [EN]
	* Returns the ordered list of component types making up this archetype.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このアーキタイプを構成する、順序付きのコンポーネント型一覧を返す。
	*/
	const DynamicArray<ComponentID>& Archetype::Layout()const
	{
		return layout_;
	}

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
	Uint32 Archetype::Index()const
	{
		return index_;
	}

	/**
	* [EN]
	* Returns the bitset signature identifying which components this
	* archetype has.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このアーキタイプがどのコンポーネントを持つかを識別する、ビットセット
	* シグネチャを返す。
	*/
	const Bitset& Archetype::Signature()const
	{
		return signature_;
	}

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
	const ArchetypeEdge& Archetype::With(ComponentID id)
	{
		SC_ASSERT(!std::ranges::contains(layout_, id), "既に持っているコンポーネントで With が呼ばれました。");

		if (id->index_ >= withEdges_.size())
		{
			withEdges_.resize(id->index_ + 1);
		}

		ResourcePtr<ArchetypeEdge>& edge = withEdges_[id->index_];
		if (edge)
		{
			return *edge;
		}

		DynamicArray<ComponentID> layout = layout_;
		layout.push_back(id);

		edge = MakePtr<ArchetypeEdge>();
		edge->target_ = ArchetypeRegistry::GetOrCreate(layout);

		/// [EN] The registry sorts layouts, so the mapping follows the target's actual column order rather than the order built here.
		/// [JP] レジストリはレイアウトを並べ替えるので、対応はここで作った順ではなく、移動先の実際の列順に合わせる。
		for (ComponentID targetID : edge->target_->Layout())
		{
			auto it = std::ranges::find(layout_, targetID);
			edge->sourceColumns_.push_back(it == layout_.end() ? UINT32_MAX : static_cast<Uint32>(it - layout_.begin()));
		}
		return *edge;
	}

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
	const ArchetypeEdge& Archetype::Without(ComponentID id)
	{
		SC_ASSERT(std::ranges::contains(layout_, id), "持っていないコンポーネントで Without が呼ばれました。");

		if (id->index_ >= withoutEdges_.size())
		{
			withoutEdges_.resize(id->index_ + 1);
		}

		ResourcePtr<ArchetypeEdge>& edge = withoutEdges_[id->index_];
		if (edge)
		{
			return *edge;
		}

		DynamicArray<ComponentID> layout = layout_;
		SeedCore::erase(layout, id);

		edge = MakePtr<ArchetypeEdge>();
		edge->target_ = ArchetypeRegistry::GetOrCreate(layout);
		edge->removedColumn_ = static_cast<Uint32>(std::ranges::find(layout_, id) - layout_.begin());

		/// [EN] Every target column exists in this archetype, since the target only lacks id.
		/// [JP] 移動先は id が無いだけなので、移動先の列はどれもこのアーキタイプにある。
		for (ComponentID targetID : edge->target_->Layout())
		{
			edge->sourceColumns_.push_back(static_cast<Uint32>(std::ranges::find(layout_, targetID) - layout_.begin()));
		}
		return *edge;
	}

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
	Chunk* Archetype::CreateChunk()
	{
		/// [EN] Chunks are large (16 KB of storage), so a recycled one is reused before allocating another.
		/// [JP] チャンクは大きい(16KB の格納領域)ので、新しく確保する前に返却済みのものを使い回す。
		return chunkPool_.Create(this);
	}

	/**
	* [EN]
	* Returns chunk to the pool for later reuse.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* chunk を後で再利用できるようプールへ返却する。
	*/
	void Archetype::RecycleChunk(Chunk* chunk)
	{
		/// [EN] The chunk stays allocated in the pool, ready for the next CreateChunk().
		/// [JP] チャンクは解放せずプールに残し、次の CreateChunk() で使えるようにする。
		chunkPool_.Recycle(chunk);
	}
}
