#include <FoundationEngine/World/ECS/Archetype/ArchetypeStorage.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs an empty storage that records locations into entities.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 場所を entities へ記録する、空のストレージを構築する。
	*/
	ArchetypeStorage::ArchetypeStorage(EntityRegistry& entities) :entities_(entities)
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns every chunk to its archetype's pool.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全チャンクをそれぞれのアーキタイプのプールへ返す。
	*/
	ArchetypeStorage::~ArchetypeStorage()
	{
		for (DynamicArray<Chunk*>& chunks : chunks_)
		{
			for (Chunk* chunk : chunks)
			{
				/// [EN] A chunk in the list is never empty, so the entity at row 0 tells which archetype owns it.
				/// [JP] 一覧にあるチャンクは空にならないので、0 行目のエンティティからどのアーキタイプのものか分かる。
				Archetype* archetype = entities_.Record(chunk->EntityAt(0))->archetype_.archetype_;
				archetype->RecycleChunk(chunk);
			}
		}
	}

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
	void* ArchetypeStorage::Add(EntityID entityID, ComponentID id)
	{
		EntityRecord* record = entities_.Record(entityID);
		if (!record || !id)
		{
			return nullptr;
		}

		ArchetypeLocation& location = record->archetype_;
		Archetype* source = location.archetype_;
		if (source)
		{
			const Bitset& signature = source->Signature();
			Size bit = ComponentRegistry::GetID(id);
			if (bit < signature.size() && signature.test(bit))
			{
				return Data(entityID, id);
			}
		}

		/// [EN] The empty archetype is process-wide and never changes, so it is looked up once; its edges lead an unplaced entity to its first archetype.
		/// [JP] 空のアーキタイプはプロセス全体で1つで変わらないので、一度だけ引く。その edge が、未配置のエンティティを最初のアーキタイプへ導く。
		static Archetype* emptyArchetype = ArchetypeRegistry::GetOrCreate(DynamicArray<ComponentID>{});
		const ArchetypeEdge& edge = (source ? source : emptyArchetype)->With(id);
		Archetype* target = edge.target_;

		if (target->Index() >= chunks_.size())
		{
			chunks_.resize(target->Index() + 1);
		}

		/// [EN] Only the last chunk can have free rows, so a full last chunk means a new one is needed.
		/// [JP] 空きがありうるのは最後のチャンクだけなので、最後が満杯なら新しいチャンクが要る。
		DynamicArray<Chunk*>& targetChunks = chunks_[target->Index()];
		if (targetChunks.empty() || targetChunks.back()->Full())
		{
			targetChunks.push_back(target->CreateChunk());
		}
		Chunk* targetChunk = targetChunks.back();

		Uint32 row = 0;
		if (source)
		{
			Chunk* sourceChunk = location.chunk_;
			Uint32 sourceRow = location.row_;
			row = targetChunk->Adopt(*sourceChunk, sourceRow, edge);
			Compact(source, sourceChunk, sourceRow, sourceChunk->Release(sourceRow));
		}
		else
		{
			row = targetChunk->Add(entityID);
		}

		location.archetype_ = target;
		location.chunk_ = targetChunk;
		location.row_ = row;
		++version_;

		Size column = static_cast<Size>(std::ranges::find(target->Layout(), id) - target->Layout().begin());
		return targetChunk->Data(column) + (targetChunk->Length(column) * row);
	}

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
	void ArchetypeStorage::Remove(EntityID entityID, ComponentID id)
	{
		EntityRecord* record = entities_.Record(entityID);
		if (!record || !record->archetype_.archetype_)
		{
			return;
		}

		ArchetypeLocation& location = record->archetype_;
		Archetype* source = location.archetype_;
		const Bitset& signature = source->Signature();
		Size bit = ComponentRegistry::GetID(id);
		if (bit >= signature.size() || !signature.test(bit))
		{
			return;
		}

		Chunk* sourceChunk = location.chunk_;
		Uint32 sourceRow = location.row_;
		const ArchetypeEdge& edge = source->Without(id);
		Archetype* target = edge.target_;

		/// [EN] Removing the last component leaves nothing to keep, so the row is destroyed outright and the entity becomes unplaced.
		/// [JP] 最後のコンポーネントを引くと残すものが無いので、行をそのまま破棄し、エンティティは未配置になる。
		if (target->Layout().empty())
		{
			Compact(source, sourceChunk, sourceRow, sourceChunk->Remove(sourceRow));
			location = ArchetypeLocation{};
			++version_;
			return;
		}

		if (target->Index() >= chunks_.size())
		{
			chunks_.resize(target->Index() + 1);
		}

		DynamicArray<Chunk*>& targetChunks = chunks_[target->Index()];
		if (targetChunks.empty() || targetChunks.back()->Full())
		{
			targetChunks.push_back(target->CreateChunk());
		}
		Chunk* targetChunk = targetChunks.back();

		Uint32 row = targetChunk->Adopt(*sourceChunk, sourceRow, edge);
		Compact(source, sourceChunk, sourceRow, sourceChunk->Release(sourceRow));

		location.archetype_ = target;
		location.chunk_ = targetChunk;
		location.row_ = row;
		++version_;
	}

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
	void ArchetypeStorage::Clear(EntityID entityID)
	{
		EntityRecord* record = entities_.Record(entityID);
		if (!record || !record->archetype_.archetype_)
		{
			return;
		}

		ArchetypeLocation& location = record->archetype_;
		Compact(location.archetype_, location.chunk_, location.row_, location.chunk_->Remove(location.row_));
		location = ArchetypeLocation{};
		++version_;
	}

	/**
	* [EN]
	* Returns entityID's component id, or nullptr if it has none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID のコンポーネント id を返す。持っていなければ nullptr。
	*/
	void* ArchetypeStorage::Data(EntityID entityID, ComponentID id)const
	{
		const EntityRecord* record = entities_.Record(entityID);
		if (!record || !record->archetype_.archetype_)
		{
			return nullptr;
		}

		const ArchetypeLocation& location = record->archetype_;
		const DynamicArray<ComponentID>& layout = location.archetype_->Layout();
		auto it = std::ranges::find(layout, id);
		if (it == layout.end())
		{
			return nullptr;
		}

		Size column = static_cast<Size>(it - layout.begin());
		return location.chunk_->Data(column) + (location.chunk_->Length(column) * location.row_);
	}

	/**
	* [EN]
	* Returns whether entityID has component id.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID がコンポーネント id を持っているかどうかを返す。
	*/
	Bool ArchetypeStorage::Has(EntityID entityID, ComponentID id)const
	{
		const EntityRecord* record = entities_.Record(entityID);
		if (!record || !record->archetype_.archetype_)
		{
			return false;
		}

		const Bitset& signature = record->archetype_.archetype_->Signature();
		Size bit = ComponentRegistry::GetID(id);
		return bit < signature.size() && signature.test(bit);
	}

	/**
	* [EN]
	* Returns the chunks of archetype in this world; empty if none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このワールドにある archetype のチャンクを返す。無ければ空。
	*/
	std::span<Chunk* const> ArchetypeStorage::Chunks(Archetype* archetype)const
	{
		if (!archetype || archetype->Index() >= chunks_.size())
		{
			return {};
		}

		const DynamicArray<Chunk*>& chunks = chunks_[archetype->Index()];
		return std::span<Chunk* const>(chunks.data(), chunks.size());
	}

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
	const DynamicArray<ComponentID>& ArchetypeStorage::Layout(EntityID entityID)const
	{
		static const DynamicArray<ComponentID> emptyLayout;

		const EntityRecord* record = entities_.Record(entityID);
		if (!record || !record->archetype_.archetype_)
		{
			return emptyLayout;
		}
		return record->archetype_.archetype_->Layout();
	}

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
	Uint32 ArchetypeStorage::Version()const
	{
		return version_;
	}

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
	void ArchetypeStorage::Compact(Archetype* archetype, Chunk* chunk, Uint32 row, EntityID movedEntityID)
	{
		/// [EN] Release moved the chunk's last row into row, so that entity's row is updated first.
		/// [JP] Release が chunk の最後の行を row へ動かしたので、まずそのエンティティの行を直す。
		if (movedEntityID != EntityID{})
		{
			entities_.Record(movedEntityID)->archetype_.row_ = row;
		}

		DynamicArray<Chunk*>& chunks = chunks_[archetype->Index()];
		Chunk* lastChunk = chunks.back();

		/// [EN] The gap now sits at chunk's end; unless chunk is the last one, the last chunk's final row is moved in so the free rows stay in the last chunk.
		/// [JP] 空きは chunk の末尾に寄った。chunk が最後のチャンクでなければ、最後のチャンクの末尾の行を移して、空きが最後のチャンクに留まるようにする。
		if (chunk != lastChunk)
		{
			Uint32 lastRow = lastChunk->EntityCount() - 1;
			EntityID lastEntityID = lastChunk->EntityAt(lastRow);
			Uint32 filledRow = chunk->Adopt(*lastChunk, lastRow);
			lastChunk->Release(lastRow);

			ArchetypeLocation& lastLocation = entities_.Record(lastEntityID)->archetype_;
			lastLocation.chunk_ = chunk;
			lastLocation.row_ = filledRow;
		}

		if (lastChunk->EntityCount() == 0)
		{
			archetype->RecycleChunk(lastChunk);
			chunks.pop_back();
		}
	}
}