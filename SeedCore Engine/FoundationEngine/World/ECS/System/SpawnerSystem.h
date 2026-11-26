#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class World;
	class CommandBuffer;

	class SpawnerSystem
	{
	public:
		void Execute(CommandBuffer& cmd, World& world, Float deltaTime);

		void Reset();

	private:
		struct SpawnedInstance
		{
			EntityID entityID_;

			Float remainingLifeTime_ = 0.0f;
		};

		struct RuntimeState
		{
			Float elapsedTime_ = 0.0f;

			DynamicArray<SpawnedInstance> instances_;
		};

		FlatMap<EntityID, RuntimeState> runtimeState_;

		std::mt19937 randomEngine_ = std::mt19937(std::random_device{}());
	};
}
