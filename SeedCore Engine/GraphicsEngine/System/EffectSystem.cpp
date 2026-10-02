#include <GraphicsEngine/System/EffectSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <GraphicsEngine/Effect/Zephyr/Effect.h>

namespace SeedCore
{
	void EffectSystem::Update(World& world, Float deltaTime)
	{
		Query<Read<Active>, Write<Effect>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, Effect& effect)
			{
				if (!active.active_)
				{
					effect.spawnCount_ = 0;
					return;
				}

				effect.age_ += deltaTime;
				effect.spawnAccumulator_ += effect.spawnRate_ * deltaTime;
				effect.spawnCount_ = static_cast<Uint32>(effect.spawnAccumulator_);
				effect.spawnAccumulator_ -= static_cast<Float>(effect.spawnCount_);
			});
	}
}