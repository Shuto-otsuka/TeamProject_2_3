#include <FoundationEngine/World/ECS/SparseSet/SparseSet.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	SparseSet::SparseSet(ComponentID id) :id_(id), epoch_(id->epoch_)
	{
		elementSize_ = ComponentRegistry::ComponentSize(id_);
		elementAlignment_ = ComponentRegistry::ComponentAlignment(id_);

		pageCapacity_ = Max<Size>(1, pageSize_ / elementSize_);
	}

	SparseSet::~SparseSet()
	{
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

		EntityID movedEntityID = entityIDs_[lastRow];
		entityIDs_[row] = movedEntityID;
		data_[row] = data_[lastRow];

		entityIDs_.pop_back();
		data_.pop_back();
		return movedEntityID;
	}

	Uint32 SparseSet::Epoch()const
	{
		return epoch_;
	}

	void* SparseSet::Data(Uint32 row)const
	{
		return data_[row];
	}

	EntityID SparseSet::EntityAt(Uint32 row)const
	{
		return entityIDs_[row];
	}

	Uint32 SparseSet::EntityCount()const
	{
		return static_cast<Uint32>(entityIDs_.size());
	}
}