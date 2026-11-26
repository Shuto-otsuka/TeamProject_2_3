#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Tag/TagRegistry.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/WorldMatrix.h>

namespace SeedCore
{
	Actor::Actor(World& world, Entity entity) : world_(&world), entity_(entity)
	{
		/// No Code
	}

	Actor::operator Bool()const
	{
		return world_ != nullptr && WorldAccessor::Entities(*world_).Exists(entity_.GetID());
	}

	Bool Actor::operator==(const Actor& other)const
	{
		return world_ == other.world_ && entity_ == other.entity_;
	}

	void* Actor::AddComponent(ComponentID id)
	{
		if (!world_ || !id)
		{
			return nullptr;
		}

		if (void* existing = world_->GetComponent(entity_, id))
		{
			return existing;
		}

		void* data = world_->AddComponent(entity_, id);
		if (!data || !id->hooks_ || !id->hooks_->upcast_)
		{
			return data;
		}

		ComponentBehaviour* behaviour = id->hooks_->upcast_(data);
		behaviour->world_ = world_;
		behaviour->entity_ = entity_;
		behaviour->id_ = id;
		if (id->hooks_->attach_)
		{
			id->hooks_->attach_(behaviour);
		}
		return data;
	}

	void* Actor::GetComponent(ComponentID id)const
	{
		if (!world_)
		{
			return nullptr;
		}
		return world_->GetComponent(entity_, id);
	}

	Bool Actor::HasComponent(ComponentID id)const
	{
		return world_ != nullptr && world_->HasComponent(entity_, id);
	}

	void Actor::RemoveComponent(ComponentID id)
	{
		if (!world_ || !id)
		{
			return;
		}

		if (id->hooks_ && id->hooks_->upcast_)
		{
			if (void* data = world_->GetComponent(entity_, id))
			{
				ComponentBehaviour* behaviour = id->hooks_->upcast_(data);
				behaviour->DispatchDestroy();
				behaviour->DispatchDetach();
			}
		}

		world_->RemoveComponent(entity_, id);
	}

	World& Actor::GetWorld()const
	{
		return *world_;
	}

	void Actor::Parent(Actor parent)
	{
		if (!world_)
		{
			return;
		}

		ActorRegistry& actors = WorldAccessor::Actors(*world_);
		const ActorRecord* record = actors.Record(entity_.GetID());
		if (!record)
		{
			return;
		}

		EntityID parentID = parent ? parent.entity_.GetID() : EntityID{};
		if (record->parent_ == parentID)
		{
			return;
		}

		actors.Attach(entity_.GetID(), parentID);
		if (record->parent_ != parentID)
		{
			return;
		}

		if (parentID != EntityID{})
		{
			if (!parent.Active())
			{
				Active(false);
			}
		}
		else
		{
			Active(true);
		}
	}

	Actor Actor::Parent()const
	{
		if (!world_)
		{
			return Actor();
		}

		const ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		if (!record)
		{
			return Actor();
		}
		return WorldAccessor::Make(*world_, record->parent_);
	}

	DynamicArray<Actor> Actor::Children()const
	{
		DynamicArray<Actor> result;
		if (!world_)
		{
			return result;
		}

		ActorRegistry& actors = WorldAccessor::Actors(*world_);
		const ActorRecord* record = actors.Record(entity_.GetID());
		if (!record)
		{
			return result;
		}

		for (EntityID child = record->child_; child != EntityID{}; child = actors.Record(child)->nextSibling_)
		{
			result.push_back(Actor(*world_, Entity(Handle<Entity>{ child.index_, child.generation_ })));
		}
		return result;
	}

	void Actor::Move(Actor after)
	{
		if (!world_)
		{
			return;
		}

		EntityID afterID = after ? after.entity_.GetID() : EntityID{};
		WorldAccessor::Actors(*world_).Move(entity_.GetID(), afterID);
	}

	const Matrix& Actor::WorldMatrix()const
	{
		static const Matrix identity = Matrix::Identity;

		const SeedCore::WorldMatrix* component = GetComponent<SeedCore::WorldMatrix>();
		if (!component)
		{
			return identity;
		}
		return component->matrix_;
	}

	void Actor::WorldMatrix(const Matrix& matrix)
	{
		SeedCore::WorldMatrix* component = GetComponent<SeedCore::WorldMatrix>();
		if (component)
		{
			component->matrix_ = matrix;
		}
	}

	Bool Actor::Active()const
	{
		const SeedCore::Active* component = GetComponent<SeedCore::Active>();
		return component != nullptr && component->active_;
	}

	void Actor::Active(Bool active)
	{
		SeedCore::Active* component = GetComponent<SeedCore::Active>();
		if (!component)
		{
			return;
		}
		component->active_ = active;

		for (Actor child : Children())
		{
			child.Active(active);
		}
	}

	void Actor::AddTag(String tag)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		Size index = TagRegistry::GetOrCreate(tag);
		if (!record || index == static_cast<Size>(-1))
		{
			return;
		}

		if (index < 64)
		{
			record->tags_ |= Uint64{ 1 } << index;
			return;
		}

		if (!record->extraTags_)
		{
			record->extraTags_ = MakePtr<Bitset>();
		}

		Size bit = index - 64;
		if (bit >= record->extraTags_->size())
		{
			record->extraTags_->resize(bit + 1);
		}
		record->extraTags_->set(bit);
	}

	Bool Actor::HasTag(String tag)const
	{
		if (!world_)
		{
			return false;
		}

		const ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		Size index = TagRegistry::Find(tag);
		if (!record || index == static_cast<Size>(-1))
		{
			return false;
		}

		if (index < 64)
		{
			return (record->tags_ & (Uint64{ 1 } << index)) != 0;
		}

		Size bit = index - 64;
		return record->extraTags_ && bit < record->extraTags_->size() && record->extraTags_->test(bit);
	}

	void Actor::RemoveTag(String tag)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		Size index = TagRegistry::Find(tag);
		if (!record || index == static_cast<Size>(-1))
		{
			return;
		}

		if (index < 64)
		{
			record->tags_ &= ~(Uint64{ 1 } << index);
			return;
		}

		Size bit = index - 64;
		if (record->extraTags_ && bit < record->extraTags_->size())
		{
			record->extraTags_->reset(bit);
		}
	}

	void Actor::Layer(Size index)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		if (record)
		{
			record->layer_ = static_cast<Uint32>(Min(index, LayerRegistry::LayerCount - 1));
		}
	}

	void Actor::Layer(const String& name)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		if (record)
		{
			record->layer_ = static_cast<Uint32>(LayerRegistry::Find(name));
		}
	}

	Size Actor::Layer()const
	{
		if (!world_)
		{
			return LayerRegistry::DefaultLayer;
		}

		const ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		return record ? record->layer_ : LayerRegistry::DefaultLayer;
	}

	const String& Actor::LayerName()const
	{
		return LayerRegistry::Name(Layer());
	}

	void* Actor::GetService(const String& type)const
	{
		if (!world_)
		{
			return nullptr;
		}
		return WorldAccessor::Services(*world_).Get(type);
	}
}
