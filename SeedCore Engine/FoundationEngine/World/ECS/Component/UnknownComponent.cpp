#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	void UnknownComponent::Keep(Actor actor, const BlueprintComponent& component)
	{
		ComponentID id = ComponentRegistry::GetComponentID<UnknownComponent>();
		if (!actor || !id)
		{
			return;
		}
		if (!actor.HasComponent(id))
		{
			actor.AddComponent(id);
		}

		UnknownComponent* unknown = static_cast<UnknownComponent*>(actor.GetComponent(id));
		if (!unknown)
		{
			return;
		}
		erase_if(unknown->components_, [&component](const BlueprintComponent& kept) { return kept.componentName_ == component.componentName_; });
		unknown->components_.push_back(component);
	}

	void UnknownComponent::Resolve(World& world)
	{
		ComponentID id = ComponentRegistry::GetComponentID<UnknownComponent>();
		if (!id)
		{
			return;
		}

		ActorRegistry& actors = WorldAccessor::Actors(world);
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			Actor actor = WorldAccessor::Make(world, current);
			UnknownComponent* unknown = static_cast<UnknownComponent*>(actor.GetComponent(id));
			if (!unknown)
			{
				continue;
			}

			DynamicArray<BlueprintComponent> kept = unknown->components_;
			DynamicArray<BlueprintComponent> remaining;
			for (const BlueprintComponent& component : kept)
			{
				ComponentID resolved = ComponentRegistry::GetComponentID(component.componentName_);
				if (!resolved)
				{
					remaining.push_back(component);
					continue;
				}

				if (!actor.HasComponent(resolved))
				{
					actor.AddComponent(resolved);
				}
				void* data = actor.GetComponent(resolved);
				if (data)
				{
					component.Apply(data);
				}
			}

			if (remaining.size() == kept.size())
			{
				continue;
			}
			if (remaining.empty())
			{
				actor.RemoveComponent(id);
				continue;
			}

			unknown = static_cast<UnknownComponent*>(actor.GetComponent(id));
			if (unknown)
			{
				unknown->components_ = remaining;
			}
		}
	}
}
