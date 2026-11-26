#include <FoundationEngine/World/ECS/System/SystemScheduler.h>
#include <FoundationEngine/World/ECS/System/TransformSystem.h>
#include <FoundationEngine/Benchmark/Benchmark.h>

#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/Spawner.h>
#include <FoundationEngine/World/ECS/Component/Lifetime.h>

namespace SeedCore
{
	template<typename Has, typename Call>
	void SystemScheduler::Dispatch(World& world, Has has, Call call)
	{
		SparseSetStorage& sparseSets = WorldAccessor::SparseSets(world);

		for (ComponentID id : ComponentRegistry::Registry())
		{
			if (!id->hooks_ || !id->hooks_->upcast_ || !has(*id->hooks_))
			{
				continue;
			}

			SparseSet* set = sparseSets.Set(id);
			if (!set || set->EntityCount() == 0)
			{
				continue;
			}

			Benchmark::Begin(id->name_);

			entities_.clear();
			for (Uint32 row = 0; row < set->EntityCount(); ++row)
			{
				entities_.push_back(set->EntityAt(row));
			}

			for (EntityID entity : entities_)
			{
				void* data = sparseSets.Data(entity, id);
				if (!data)
				{
					continue;
				}

				if (!WorldAccessor::Make(world, entity).Active())
				{
					continue;
				}

				call(id->hooks_->upcast_(data));
			}

			Benchmark::End(id->name_);
		}
	}

	void SystemScheduler::Run(World& world, ResourceCache& cache, JobExecutor& executor, Float worldElapsedTime, Float gameElapsedTime, Bool isPlaying)
	{
		if (isPlaying)
		{
			Benchmark::Begin("Transform");
			transformSystem_.Execute(world);
			Benchmark::End("Transform");

			Benchmark::Begin("Awake");
			Dispatch(world, [](const ComponentHooks&) { return true; }, [](ComponentBehaviour* component)
				{
					if (!component->awoken_)
					{
						component->awoken_ = true;
						component->DispatchAwake();
					}
				});
			Benchmark::End("Awake");

			Benchmark::Begin("Start");
			Dispatch(world, [](const ComponentHooks&) { return true; }, [](ComponentBehaviour* component)
				{
					if (!component->started_)
					{
						component->started_ = true;
						component->DispatchStart();
					}
				});
			Benchmark::End("Start");
		}

		if (isPlaying)
		{
			systemGraph_.Clear();
			systemGraph_.Add(Query<Read<Spawner>, Read<Lifetime>>::ReadSignature(), Query<Read<Spawner>, Read<Lifetime>>::WriteSignature(), [this, &world, gameElapsedTime]()
				{
					Benchmark::Begin("Spawner");
					spawnerSystem_.Execute(commandBuffer_, world, gameElapsedTime);
					Benchmark::End("Spawner");

					Benchmark::Begin("Lifetime");
					lifetimeSystem_.Execute(commandBuffer_, world, gameElapsedTime);
					Benchmark::End("Lifetime");
				});

			Benchmark::Begin("SystemGraph");
			systemGraph_.Run(executor);
			Benchmark::End("SystemGraph");
		}

		Benchmark::Begin("CommandBuffer");
		commandBuffer_.Flush(world, cache);
		Benchmark::End("CommandBuffer");

		Benchmark::Begin("Transform");
		transformSystem_.Execute(world);
		Benchmark::End("Transform");

		if (isPlaying)
		{
			Benchmark::Begin("Tick");
			Dispatch(world, [](const ComponentHooks& hooks) { return hooks.tick_ != nullptr; }, [gameElapsedTime](ComponentBehaviour* component)
				{
					component->DispatchTick(gameElapsedTime);
				});
			Benchmark::End("Tick");

			Benchmark::Begin("LateTick");
			Dispatch(world, [](const ComponentHooks& hooks) { return hooks.lateTick_ != nullptr; }, [gameElapsedTime](ComponentBehaviour* component)
				{
					component->DispatchLateTick(gameElapsedTime);
				});
			Benchmark::End("LateTick");
		}
		else
		{
			Benchmark::Begin("EditorTick");
			Dispatch(world, [](const ComponentHooks& hooks) { return hooks.editorTick_ != nullptr; }, [worldElapsedTime](ComponentBehaviour* component)
				{
					component->DispatchEditorTick(worldElapsedTime);
				});
			Benchmark::End("EditorTick");
		}
	}

	void SystemScheduler::Step(World& world, Float fixedTime)
	{
		Benchmark::Begin("FixedTick");
		Dispatch(world, [](const ComponentHooks& hooks) { return hooks.fixedTick_ != nullptr; }, [fixedTime](ComponentBehaviour* component)
			{
				if (component->started_)
				{
					component->DispatchFixedTick(fixedTime);
				}
			});
		Benchmark::End("FixedTick");
	}

	void SystemScheduler::Reset()
	{
		spawnerSystem_.Reset();
		lifetimeSystem_.Reset();
		commandBuffer_.Clear();
	}
}
