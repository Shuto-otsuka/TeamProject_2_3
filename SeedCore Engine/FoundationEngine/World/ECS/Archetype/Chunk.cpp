#include <FoundationEngine/World/ECS/Archetype/Chunk.h>
#include <FoundationEngine/World/ECS/Archetype/Archetype.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	Chunk::Chunk(Archetype* archetype) : archetype_(archetype)
	{
		const auto& layout = archetype_->Layout();
		componentCount_ = layout.size();

		Size totalSize = 0;
		Size alignmentSlack = 0;
		for (Size index = 0; index < componentCount_; ++index)
		{
			ComponentID id = layout[index];
			totalSize += ComponentRegistry::ComponentSize(id);

			Size alignment = ComponentRegistry::ComponentAlignment(id);
			alignmentSlack += (alignment > 0) ? (alignment - 1) : 0;
		}

		if (totalSize == 0)
		{
			capacity_ = maxEntitiesPerChunk_;
		}
		else
		{
			Size usable = (chunkSize_ > alignmentSlack) ? (chunkSize_ - alignmentSlack) : 0;
			capacity_ = usable / totalSize;

			if (capacity_ > maxEntitiesPerChunk_)
			{
				capacity_ = maxEntitiesPerChunk_;
			}
		}

		Uint8* cursor = buffer_;
		for (Size index = 0; index < componentCount_; ++index)
		{
			ComponentID id = layout[index];
			Size size = ComponentRegistry::ComponentSize(id);
			Size alignment = ComponentRegistry::ComponentAlignment(id);

			Size padding = (alignment - (reinterpret_cast<std::uintptr_t>(cursor) % alignment)) % alignment;
			cursor += padding;

			componentSizes_[index] = size;
			componentDataPointers_[index] = cursor;

			cursor += (size * capacity_);
		}
	}

	Chunk::~Chunk()
	{
		for (Uint32 entityIndex = 0; entityIndex < entityCount_; ++entityIndex)
		{
			for (Size componentIndex = 0; componentIndex < componentCount_; ++componentIndex)
			{
				const ComponentMetadata& meta = ComponentRegistry::Get(archetype_->Layout()[componentIndex]);
				Uint8* slot = componentDataPointers_[componentIndex] + (componentSizes_[componentIndex] * entityIndex);
				meta.destruct_(slot);
			}
		}
	}

	Uint32 Chunk::Add(EntityID entityID)
	{
		SC_ASSERT(entityCount_ < capacity_, "満杯のチャンクに Add されました。呼び出し側で Full() を確認してチャンクを用意してください。");

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

	EntityID Chunk::Release(Uint32 row)
	{
		if (row >= entityCount_)
		{
			return EntityID{};
		}

		Uint32 lastRow = entityCount_ - 1;
		if (row == lastRow)
		{
			--entityCount_;
			return EntityID{};
		}

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

	Uint32 Chunk::Adopt(Chunk& source, Uint32 sourceRow)
	{
		SC_ASSERT(entityCount_ < capacity_, "満杯のチャンクに Adopt されました。呼び出し側で Full() を確認して新しいチャンクを用意してください。");
		SC_ASSERT(source.archetype_ == archetype_, "edge 無しの Adopt は同じアーキタイプのチャンク同士でしか使えません。");

		Uint32 row = entityCount_;
		entityIDs_[row] = source.entityIDs_[sourceRow];

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

	Uint32 Chunk::Adopt(Chunk& source, Uint32 sourceRow, const ArchetypeEdge& edge)
	{
		SC_ASSERT(entityCount_ < capacity_, "満杯のチャンクに Adopt されました。呼び出し側で Full() を確認して新しいチャンクを用意してください。");

		Uint32 row = entityCount_;
		entityIDs_[row] = source.entityIDs_[sourceRow];

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

		if (edge.removedColumn_ != UINT32_MAX)
		{
			const ComponentMetadata& meta = ComponentRegistry::Get(source.archetype_->Layout()[edge.removedColumn_]);
			Uint8* sourceSlot = source.componentDataPointers_[edge.removedColumn_] + (source.componentSizes_[edge.removedColumn_] * sourceRow);
			meta.destruct_(sourceSlot);
		}

		++entityCount_;
		return row;
	}

	Uint8* Chunk::Data(Size layoutIndex)
	{
		return componentDataPointers_[layoutIndex];
	}

	Size Chunk::Length(Size layoutIndex)const
	{
		return componentSizes_[layoutIndex];
	}

	Bool Chunk::Full()const
	{
		return entityCount_ >= capacity_;
	}

	Size Chunk::Capacity()const
	{
		return capacity_;
	}

	EntityID Chunk::EntityAt(Size index)const
	{
		return entityIDs_[index];
	}

	Uint32 Chunk::EntityCount()const
	{
		return entityCount_;
	}
}
