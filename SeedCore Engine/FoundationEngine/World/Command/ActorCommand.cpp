#include <FoundationEngine/World/Command/ActorCommand.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	ActorCreateCommand::ActorCreateCommand(World& world, ResourceCache& cache, DynamicArray<BlueprintNode> nodes, Uint32 parentPersistentId, Uint32 prevSiblingPersistentId) : world_(world), cache_(cache), nodes_(std::move(nodes)), parentPersistentId_(parentPersistentId), prevSiblingPersistentId_(prevSiblingPersistentId)
	{
		/// No Code
	}

	void ActorCreateCommand::Redo()
	{
		Actor rootParent = parentPersistentId_ ? WorldAccessor::Find(world_, parentPersistentId_) : Actor();

		DynamicArray<Actor> instantiated = BlueprintNode::Instantiate(world_, cache_, nodes_, rootParent, false);

		if (rootParent && !instantiated.empty() && instantiated[0])
		{
			instantiated[0].Move(WorldAccessor::Find(world_, prevSiblingPersistentId_));
		}
	}

	void ActorCreateCommand::Undo()
	{
		for (Size index = nodes_.size(); index > 0; --index)
		{
			Actor actor = WorldAccessor::Find(world_, nodes_[index - 1].persistentId_);
			if (actor)
			{
				world_.DestroyActor(actor);
			}
		}
	}

	ActorDeleteCommand::ActorDeleteCommand(World& world, ResourceCache& cache, Actor actor) : world_(world), cache_(cache)
	{
		DynamicArray<BlueprintNode> nodes;
		BlueprintNode::Capture(actor, nodes);
		node_ = nodes[0];

		ActorRegistry& actors = WorldAccessor::Actors(world_);
		persistentId_ = actors.Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_;

		Actor parent = actor.Parent();
		parentPersistentId_ = parent ? actors.Record(ActorAccessor::GetEntity(parent).GetID())->persistentID_ : 0;

		std::ranges::transform(actor.Children(), std::back_inserter(childPersistentIds_), [&actors](const Actor& child) { return actors.Record(ActorAccessor::GetEntity(child).GetID())->persistentID_; });
	}

	void ActorDeleteCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, persistentId_);
		if (actor)
		{
			world_.DestroyActor(actor);
		}
	}

	void ActorDeleteCommand::Undo()
	{
		Actor parent = parentPersistentId_ ? WorldAccessor::Find(world_, parentPersistentId_) : Actor();

		Actor actor = node_.Instantiate(world_, cache_, parent, false);
		if (!actor)
		{
			return;
		}

		for (Uint32 childPersistentId : childPersistentIds_)
		{
			Actor child = WorldAccessor::Find(world_, childPersistentId);
			if (child)
			{
				child.Parent(actor);
			}
		}
	}

	ActorReparentCommand::ActorReparentCommand(World& world, Uint32 actorPersistentId, Uint32 oldParentPersistentId, Uint32 oldPrevSiblingPersistentId, Uint32 newParentPersistentId) : world_(world), actorPersistentId_(actorPersistentId), oldParentPersistentId_(oldParentPersistentId), oldPrevSiblingPersistentId_(oldPrevSiblingPersistentId), newParentPersistentId_(newParentPersistentId)
	{
		/// No Code
	}

	void ActorReparentCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		Actor newParent = newParentPersistentId_ ? WorldAccessor::Find(world_, newParentPersistentId_) : Actor();
		actor.Parent(newParent);
	}

	void ActorReparentCommand::Undo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		Actor oldParent = oldParentPersistentId_ ? WorldAccessor::Find(world_, oldParentPersistentId_) : Actor();
		actor.Parent(oldParent);

		if (oldParent)
		{
			actor.Move(WorldAccessor::Find(world_, oldPrevSiblingPersistentId_));
		}
	}

	ActorTagCommand::ActorTagCommand(World& world, Uint32 actorPersistentId, const String& tag, Bool addOnRedo) : world_(world), actorPersistentId_(actorPersistentId), tag_(tag), addOnRedo_(addOnRedo)
	{
		/// No Code
	}

	void ActorTagCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		if (addOnRedo_)
		{
			actor.AddTag(tag_);
		}
		else
		{
			actor.RemoveTag(tag_);
		}
	}

	void ActorTagCommand::Undo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		if (addOnRedo_)
		{
			actor.RemoveTag(tag_);
		}
		else
		{
			actor.AddTag(tag_);
		}
	}

	ActorLayerCommand::ActorLayerCommand(World& world, Uint32 actorPersistentId, const String& oldLayerName, const String& newLayerName) : world_(world), actorPersistentId_(actorPersistentId), oldLayerName_(oldLayerName), newLayerName_(newLayerName)
	{
		/// No Code
	}

	void ActorLayerCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		actor.Layer(newLayerName_);
	}

	void ActorLayerCommand::Undo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		actor.Layer(oldLayerName_);
	}

	ActorActiveCommand::ActorActiveCommand(World& world, Actor actor, Bool newActive) : world_(world), rootPersistentId_(WorldAccessor::Actors(world).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_), newActive_(newActive)
	{
		ActorRegistry& actors = WorldAccessor::Actors(world_);

		DynamicArray<Actor> pending;
		pending.push_back(actor);

		while (!pending.empty())
		{
			Actor current = pending.back();
			pending.pop_back();

			Entry entry;
			entry.persistentId_ = actors.Record(ActorAccessor::GetEntity(current).GetID())->persistentID_;
			entry.oldActive_ = current.Active();
			entries_.push_back(entry);

			std::ranges::copy(current.Children(), std::back_inserter(pending));
		}
	}

	void ActorActiveCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, rootPersistentId_);
		if (actor)
		{
			actor.Active(newActive_);
		}
	}

	void ActorActiveCommand::Undo()
	{
		static const String activeString("Active");
		ComponentID activeID = ComponentRegistry::GetComponentID(activeString);

		for (const Entry& entry : entries_)
		{
			Actor actor = WorldAccessor::Find(world_, entry.persistentId_);
			if (!actor)
			{
				continue;
			}

			Active* component = static_cast<Active*>(actor.GetComponent(activeID));
			if (component)
			{
				component->active_ = entry.oldActive_;
			}
		}
	}
}
