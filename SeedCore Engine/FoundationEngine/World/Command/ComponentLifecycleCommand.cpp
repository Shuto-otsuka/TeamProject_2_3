#include <FoundationEngine/World/Command/ComponentLifecycleCommand.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	ComponentAddCommand::ComponentAddCommand(World& world, Uint32 actorPersistentId, ComponentID componentID)
		: world_(world), actorPersistentId_(actorPersistentId), componentID_(componentID)
	{
		/// No Code
	}

	void ComponentAddCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (actor)
		{
			actor.AddComponent(componentID_);
		}
	}

	void ComponentAddCommand::Undo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (actor)
		{
			actor.RemoveComponent(componentID_);
		}
	}

	ComponentRemoveCommand::ComponentRemoveCommand(World& world, Uint32 actorPersistentId, ComponentID componentID, const String& componentName, void* componentData)
		: world_(world), actorPersistentId_(actorPersistentId), componentID_(componentID), captured_(BlueprintComponent::Capture(componentName, componentData))
	{
		/// No Code
	}

	void ComponentRemoveCommand::Redo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (actor)
		{
			actor.RemoveComponent(componentID_);
		}
	}

	void ComponentRemoveCommand::Undo()
	{
		Actor actor = WorldAccessor::Find(world_, actorPersistentId_);
		if (!actor)
		{
			return;
		}

		actor.AddComponent(componentID_);

		void* componentData = actor.GetComponent(componentID_);
		if (componentData)
		{
			captured_.Apply(componentData);
		}
	}
}
