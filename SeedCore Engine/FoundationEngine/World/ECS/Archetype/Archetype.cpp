#include <FoundationEngine/World/ECS/Archetype/Archetype.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/World/ECS/Archetype/Chunk.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	Archetype::Archetype(const DynamicArray<ComponentID>& layout, Uint32 index) : layout_(layout), index_(index)
	{
		for (ComponentID id : layout)
		{
			Size internalID = ComponentRegistry::GetID(id);

			if (internalID >= signature_.size())
			{
				signature_.resize(internalID + 1);
			}

			signature_.set(internalID);
		}
	}

	const DynamicArray<ComponentID>& Archetype::Layout()const
	{
		return layout_;
	}

	Uint32 Archetype::Index()const
	{
		return index_;
	}

	const Bitset& Archetype::Signature()const
	{
		return signature_;
	}

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

		for (ComponentID targetID : edge->target_->Layout())
		{
			auto it = std::ranges::find(layout_, targetID);
			edge->sourceColumns_.push_back(it == layout_.end() ? UINT32_MAX : static_cast<Uint32>(it - layout_.begin()));
		}
		return *edge;
	}

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

		for (ComponentID targetID : edge->target_->Layout())
		{
			edge->sourceColumns_.push_back(static_cast<Uint32>(std::ranges::find(layout_, targetID) - layout_.begin()));
		}
		return *edge;
	}

	Chunk* Archetype::CreateChunk()
	{
		return chunkPool_.Create(this);
	}

	void Archetype::RecycleChunk(Chunk* chunk)
	{
		chunkPool_.Recycle(chunk);
	}
}
