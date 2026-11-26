#include <FoundationEngine/World/ECS/Archetype/ArchetypeStorage.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	ArchetypeStorage::ArchetypeStorage(EntityRegistry& entities) :entities_(entities)
	{
		/// No Code
	}

	ArchetypeStorage::~ArchetypeStorage()
	{
		for (DynamicArray<Chunk*>& chunks : chunks_)
		{
			for (Chunk* chunk : chunks)
			{
				Archetype* archetype = entities_.Record(chunk->EntityAt(0))->archetype_.archetype_;
				archetype->RecycleChunk(chunk);
			}
		}
	}

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

		static Archetype* emptyArchetype = ArchetypeRegistry::GetOrCreate(DynamicArray<ComponentID>{});
		const ArchetypeEdge& edge = (source ? source : emptyArchetype)->With(id);
		Archetype* target = edge.target_;

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

	std::span<Chunk* const> ArchetypeStorage::Chunks(Archetype* archetype)const
	{
		if (!archetype || archetype->Index() >= chunks_.size())
		{
			return {};
		}

		const DynamicArray<Chunk*>& chunks = chunks_[archetype->Index()];
		return std::span<Chunk* const>(chunks.data(), chunks.size());
	}

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

	Uint32 ArchetypeStorage::Version()const
	{
		return version_;
	}

	void ArchetypeStorage::Compact(Archetype* archetype, Chunk* chunk, Uint32 row, EntityID movedEntityID)
	{
		if (movedEntityID != EntityID{})
		{
			entities_.Record(movedEntityID)->archetype_.row_ = row;
		}

		DynamicArray<Chunk*>& chunks = chunks_[archetype->Index()];
		Chunk* lastChunk = chunks.back();

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