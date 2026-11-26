#include <FoundationEngine/World/Actor/ActorRegistry.h>

namespace SeedCore
{
	ActorRegistry::ActorRegistry(EntityRegistry& entities) :entities_(entities)
	{
		/// No Code
	}

	void ActorRegistry::Add(EntityID entityID, Uint32 persistentID)
	{
		if (!entities_.Exists(entityID))
		{
			return;
		}

		if (entityID.index_ >= records_.size())
		{
			records_.resize(entityID.index_ + 1);
		}
		records_[entityID.index_] = ActorRecord{};

		if (persistentID == 0 || (persistentID < persistentIndex_.size() && persistentIndex_[persistentID] != EntityID{}))
		{
			persistentID = nextPersistentID_++;
		}
		else
		{
			nextPersistentID_ = Max(nextPersistentID_, persistentID + 1);
		}

		if (persistentID >= persistentIndex_.size())
		{
			persistentIndex_.resize(persistentID + 1);
		}
		persistentIndex_[persistentID] = entityID;
		records_[entityID.index_].persistentID_ = persistentID;

		EntityID last = (root_ != EntityID{}) ? records_[root_.index_].previousSibling_ : EntityID{};
		Link(entityID, EntityID{}, last);
	}

	void ActorRegistry::Remove(EntityID entityID)
	{
		ActorRecord* record = Record(entityID);
		if (!record)
		{
			return;
		}

		SC_ASSERT(record->child_ == EntityID{}, "子が残っている actor を Remove しようとしました。子を先に取り除いてください。");

		Unlink(entityID);
		persistentIndex_[record->persistentID_] = EntityID{};

		*record = ActorRecord{};
	}

	void ActorRegistry::Attach(EntityID child, EntityID parent)
	{
		if (!Record(child) || (parent != EntityID{} && !Record(parent)))
		{
			return;
		}

		for (EntityID ancestor = parent;ancestor != EntityID{};ancestor = records_[ancestor.index_].parent_)
		{
			if (ancestor == child)
			{
				return;
			}
		}

		Unlink(child);

		EntityID head = (parent != EntityID{}) ? records_[parent.index_].child_ : root_;
		EntityID last = (head != EntityID{}) ? records_[head.index_].previousSibling_ : EntityID{};
		Link(child, parent, last);
	}

	void ActorRegistry::Move(EntityID child, EntityID after)
	{
		ActorRecord* record = Record(child);
		if (!record || after == child)
		{
			return;
		}

		if (after != EntityID{})
		{
			ActorRecord* afterRecord = Record(after);
			if (!afterRecord || afterRecord->parent_ != record->parent_)
			{
				return;
			}
		}

		Unlink(child);
		Link(child, record->parent_, after);
	}

	EntityID ActorRegistry::Root()const
	{
		return root_;
	}

	EntityID ActorRegistry::Next(EntityID entityID)const
	{
		const ActorRecord* record = Record(entityID);
		if (!record)
		{
			return EntityID{};
		}

		if (record->child_ != EntityID{})
		{
			return record->child_;
		}

		for (EntityID current = entityID;current != EntityID{};current = records_[current.index_].parent_)
		{
			EntityID sibling = records_[current.index_].nextSibling_;
			if (sibling != EntityID{})
			{
				return sibling;
			}
		}
		return EntityID{};
	}

	EntityID ActorRegistry::Find(Uint32 persistentID)const
	{
		if (persistentID >= persistentIndex_.size())
		{
			return EntityID{};
		}
		return persistentIndex_[persistentID];
	}

	void ActorRegistry::Link(EntityID child, EntityID parent, EntityID after)
	{
		ActorRecord& record = records_[child.index_];
		record.parent_ = parent;

		EntityID& head = (parent != EntityID{}) ? records_[parent.index_].child_ : root_;

		if (head == EntityID{})
		{
			head = child;
			record.previousSibling_ = child;
			record.nextSibling_ = EntityID{};
			return;
		}

		ActorRecord& first = records_[head.index_];

		if (after == EntityID{})
		{
			record.previousSibling_ = first.previousSibling_;
			record.nextSibling_ = head;
			first.previousSibling_ = child;
			head = child;
			return;
		}

		ActorRecord& afterRecord = records_[after.index_];
		record.previousSibling_ = after;
		record.nextSibling_ = afterRecord.nextSibling_;

		if (afterRecord.nextSibling_ != EntityID{})
		{
			records_[afterRecord.nextSibling_.index_].previousSibling_ = child;
		}
		else
		{
			first.previousSibling_ = child;
		}
		afterRecord.nextSibling_ = child;
	}

	void ActorRegistry::Unlink(EntityID child)
	{
		ActorRecord& record = records_[child.index_];
		EntityID& head = (record.parent_ != EntityID{}) ? records_[record.parent_.index_].child_ : root_;

		if (head == child)
		{
			if (record.nextSibling_ != EntityID{})
			{
				records_[record.nextSibling_.index_].previousSibling_ = record.previousSibling_;
			}
			head = record.nextSibling_;
		}
		else
		{
			records_[record.previousSibling_.index_].nextSibling_ = record.nextSibling_;

			if (record.nextSibling_ != EntityID{})
			{
				records_[record.nextSibling_.index_].previousSibling_ = record.previousSibling_;
			}
			else
			{
				records_[head.index_].previousSibling_ = record.previousSibling_;
			}
		}

		record.nextSibling_ = EntityID{};
		record.previousSibling_ = EntityID{};
	}
}