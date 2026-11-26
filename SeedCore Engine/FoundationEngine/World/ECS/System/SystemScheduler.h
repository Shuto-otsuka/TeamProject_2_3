#pragma once
#include <FoundationEngine/World/ECS/System/TransformSystem.h>
#include <FoundationEngine/World/ECS/System/SpawnerSystem.h>
#include <FoundationEngine/World/ECS/System/LifetimeSystem.h>
#include <FoundationEngine/World/ECS/System/SystemGraph.h>
#include <FoundationEngine/World/Command/CommandBuffer.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class JobExecutor;

	class SEEDCORE_API SystemScheduler
	{
	public:
		SystemScheduler() = default;

		~SystemScheduler() = default;

		void Run(World& world, ResourceCache& cache, JobExecutor& executor, Float worldElapsedTime, Float gameElapsedTime, Bool isPlaying = true);

		void Step(World& world, Float fixedTime);

		void Reset();

	private:
		template<typename Has, typename Call>
		void Dispatch(World& world, Has has, Call call);

	private:
		TransformSystem transformSystem_;

		SpawnerSystem spawnerSystem_;

		LifetimeSystem lifetimeSystem_;

		CommandBuffer commandBuffer_;

		SystemGraph systemGraph_;

		DynamicArray<EntityID> entities_;
	};
}
