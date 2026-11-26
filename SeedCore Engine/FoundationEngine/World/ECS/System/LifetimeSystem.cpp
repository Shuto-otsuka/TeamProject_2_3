#include <FoundationEngine/World/ECS/System/LifetimeSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Command/CommandBuffer.h>
#include <FoundationEngine/World/ECS/Component/Lifetime.h>

namespace SeedCore
{
	void LifetimeSystem::Execute(CommandBuffer& cmd, World& world, Float deltaTime)
	{
		Query<Read<Lifetime>> query(world);
		query.ForEach([&](EntityID entityID, const Lifetime& lifetime)
			{
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor || !actor.Active())
				{
					return;
				}

				if (!remaining_.contains(entityID))
				{
					remaining_[entityID] = lifetime.duration_;
				}

				Float& remaining = remaining_[entityID];
				remaining -= deltaTime;
				if (remaining <= 0.0f)
				{
					cmd.DestroyEntity(entityID);
					remaining_.erase(entityID);
				}
			});
	}

	void LifetimeSystem::Reset()
	{
		remaining_.clear();
	}
}
