#include <FoundationEngine/World/ECS/System/SpawnerSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Command/CommandBuffer.h>
#include <FoundationEngine/World/ECS/Component/Spawner.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	void SpawnerSystem::Execute(CommandBuffer& cmd, World& world, Float deltaTime)
	{
		Query<Read<Spawner>> query(world);
		query.ForEach([&](EntityID entityID, const Spawner& spawner)
			{
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor || !actor.Active())
				{
					return;
				}

				RuntimeState& state = runtimeState_[entityID];

				for (SpawnedInstance& instance : state.instances_)
				{
					if (CommandBuffer::Provisional(instance.entityID_))
					{
						EntityID resolved = cmd.Resolved(instance.entityID_);
						if (resolved != EntityID{})
						{
							instance.entityID_ = resolved;
						}
					}
				}

				for (Size instanceIndex = 0; instanceIndex < state.instances_.size(); )
				{
					SpawnedInstance& instance = state.instances_[instanceIndex];
					instance.remainingLifeTime_ -= deltaTime;

					Bool stillProvisional = CommandBuffer::Provisional(instance.entityID_);
					Bool destroyedElsewhere = !stillProvisional && !WorldAccessor::Make(world, instance.entityID_);

					if (instance.remainingLifeTime_ > 0.0f && !destroyedElsewhere)
					{
						instanceIndex++;
						continue;
					}

					if (instance.remainingLifeTime_ <= 0.0f && !stillProvisional && !destroyedElsewhere)
					{
						cmd.DestroyEntity(instance.entityID_);
					}

					state.instances_[instanceIndex] = state.instances_.back();
					state.instances_.pop_back();
				}

				if (!spawner.autoStart_)
				{
					return;
				}

				if (static_cast<Int>(state.instances_.size()) >= spawner.maxCount_)
				{
					return;
				}

				state.elapsedTime_ += deltaTime;
				if (state.elapsedTime_ < spawner.spawnInterval_)
				{
					return;
				}

				state.elapsedTime_ -= spawner.spawnInterval_;

				if (spawner.prefabID_ == 0)
				{
					return;
				}

				Vector3 spawnPosition = actor.WorldMatrix().Translation();
				if (spawner.randomSpawn_)
				{
					std::uniform_real_distribution<Float> jitter(-spawner.randomRadius_, spawner.randomRadius_);
					spawnPosition.x += jitter(randomEngine_);
					spawnPosition.y += jitter(randomEngine_);
					spawnPosition.z += jitter(randomEngine_);
				}

				SpawnedInstance instance;
				instance.entityID_ = cmd.SpawnPrefab(spawner.prefabID_, spawnPosition);
				instance.remainingLifeTime_ = spawner.lifeTime_;
				state.instances_.push_back(instance);
			});
	}

	void SpawnerSystem::Reset()
	{
		runtimeState_.clear();
	}
}
