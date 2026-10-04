#include <FoundationEngine/World/ECS/System/SystemScheduler.h>
#include <FoundationEngine/World/ECS/System/TransformSystem.h>
#include <FoundationEngine/World/ECS/System/MoveSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>
#include <FoundationEngine/World/ECS/Component/Spawner.h>
#include <FoundationEngine/World/ECS/Component/Lifetime.h>

namespace SeedCore
{
	/**
	* [EN]
	* Runs one frame: drives Awake/Start (if isPlaying), then (if
	* isPlaying) runs MoveSystem and the structural systems (Spawner +
	* Lifetime) through SystemGraph on executor - MoveSystem in parallel
	* with the structural pair since their component accesses do not
	* conflict - flushes the recorded structural changes, runs the
	* built-in TransformSystem (all before Tick/LateTick so this frame's
	* motion and any newly spawned actor's position are in this frame's
	* world matrix), then drives Tick/LateTick (if isPlaying). Inactive
	* actors receive none of Awake/Start/Tick/LateTick.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1フレーム分を実行する: （isPlaying であれば）Awake/Start を駆動し、
	* （isPlaying であれば）MoveSystem と構造系システム（Spawner +
	* Lifetime）を SystemGraph 経由で executor 上で実行する - MoveSystem は
	* コンポーネントアクセスが衝突しないため構造系ペアと並列に走る -
	* 記録された構造変更を flush し、組み込みの TransformSystem を実行し
	* （すべて Tick/LateTick より前 — 今フレームの移動や新しく生成された
	* actor の位置が同じフレームのワールド行列に入るように）、
	* （isPlaying であれば）Tick/LateTick を駆動する。非アクティブな
	* actor には Awake/Start/Tick/LateTick のどれも送らない。
	*/
	void SystemScheduler::Run(World& world, ResourceCache& cache, JobExecutor& executor, Float elapsedTime, Bool isPlaying)
	{
		if (isPlaying)
		{
			/// [EN] Every pass below walks a copy of the actor list: a callback may spawn or destroy actors, which reallocates or reorders the World's own list mid-loop. Actors spawned during a pass are picked up from the next frame.
			/// [JP] 以下の各パスは actor 一覧のコピーを回す。コールバックが actor を生成・破棄すると、World 自身の一覧がループ途中で再確保されたり並び替わったりするため。パス中に生成された actor は次のフレームから処理される。
			DynamicArray<Actor> awakeActors = world.GetActors();
			for (const Actor& listedActor : awakeActors)
			{
				/// [EN] An earlier callback in this pass may have destroyed this actor.
				/// [JP] このパスの先のコールバックが、この actor を破棄しているかもしれない。
				Actor actor = world.GetActor(listedActor.GetEntity());
				if (!actor || !actor.Active())
				{
					continue;
				}

				Entity entity = actor.GetEntity();
				EntityID entityID = entity.GetID();

				/// [EN] The component list is re-read by index each step, since a callback may add or remove components or reallocate the record holding the list.
				/// [JP] コールバックがコンポーネントを追加・削除したり、一覧を持つ記録を再確保したりし得るため、コンポーネント一覧は毎回インデックスで読み直す。
				for (Size componentIndex = 0; componentIndex < actor.ComponentIDList().size(); ++componentIndex)
				{
					ComponentID id = actor.ComponentIDList()[componentIndex];
					void* data = world.GetComponent(entityID, id);
					if (!data)
					{
						continue;
					}

					ComponentBehaviour* component = static_cast<ComponentBehaviour*>(data);
					if (!component->awoken_)
					{
						component->awoken_ = true;
						if (component->awake_)
						{
							component->awake_(component);
						}
					}

					/// [EN] The callback may have destroyed its own actor, which leaves no component list to read.
					/// [JP] コールバックが自分の actor を破棄した場合、読むべきコンポーネント一覧はもう無い。
					if (!world.GetActor(entityID))
					{
						break;
					}
				}
			}

			DynamicArray<Actor> startActors = world.GetActors();
			for (const Actor& listedActor : startActors)
			{
				Actor actor = world.GetActor(listedActor.GetEntity());
				if (!actor || !actor.Active())
				{
					continue;
				}

				Entity entity = actor.GetEntity();
				EntityID entityID = entity.GetID();

				for (Size componentIndex = 0; componentIndex < actor.ComponentIDList().size(); ++componentIndex)
				{
					ComponentID id = actor.ComponentIDList()[componentIndex];
					void* data = world.GetComponent(entityID, id);
					if (!data)
					{
						continue;
					}

					ComponentBehaviour* component = static_cast<ComponentBehaviour*>(data);
					if (!component->started_)
					{
						component->started_ = true;
						if (component->start_)
						{
							component->start_(component);
						}
					}

					if (!world.GetActor(entityID))
					{
						break;
					}
				}
			}
		}

		/// [EN] Spawn before TransformSystem::Execute so a new actor's world matrix is built from its spawn Position this frame;
		///      Rigidbody::OnAwake places its body from the world matrix, not from Position.
		/// [JP] TransformSystem::Execute より前にスポーンし、新しい actor のワールド行列をこのフレームのうちにスポーン位置から作る。
		///      Rigidbody::OnAwake はボディを Position ではなくワールド行列から配置するため。
		if (isPlaying)
		{
			/// [EN] MoveSystem (reads Velocity, writes Position) has no component-access conflict with the structural systems, so SystemGraph runs them in parallel on the executor. SpawnerSystem and LifetimeSystem share one CommandBuffer, so they are registered as a single serial task rather than two.
			/// [JP] MoveSystem(Velocity を読み Position を書く)は構造系システムとコンポーネントアクセスが衝突しないため、SystemGraph が executor 上で並列に走らせる。SpawnerSystem と LifetimeSystem は1つの CommandBuffer を共有するので、2つではなく1つの直列タスクとして登録する。
			systemGraph_.Clear();
			systemGraph_.Add(
				Query<Read<Velocity>, Write<Position>>::GetReadSignature(),
				Query<Read<Velocity>, Write<Position>>::GetWriteSignature(),
				[this, &world, elapsedTime]() { moveSystem_.Execute(world, elapsedTime); });
			systemGraph_.Add(
				Query<Read<Spawner>, Read<Lifetime>>::GetReadSignature(),
				Query<Read<Spawner>, Read<Lifetime>>::GetWriteSignature(),
				[this, &world, elapsedTime]()
				{
					spawnerSystem_.Execute(commandBuffer_, world, elapsedTime);
					lifetimeSystem_.Execute(commandBuffer_, world, elapsedTime);
				});
			systemGraph_.Run(executor);
		}

		/// [EN] Flush the structural changes systems recorded (spawns, destroys) before TransformSystem, so this frame's world matrices already reflect them.
		/// [JP] システムが記録した構造変更(スポーン・破棄)を TransformSystem より前に flush し、今フレームのワールド行列に反映させる。
		commandBuffer_.Flush(world, cache);

		transformSystem_.Execute(world);

		if (isPlaying)
		{
			/// [EN] Same copy-and-recheck walk as Awake/Start above, since Tick is where gameplay most often spawns and destroys actors.
			/// [JP] 上の Awake/Start と同じく、コピーを回して存在を確かめ直す。Tick はゲームプレイが最もよく actor を生成・破棄する場所であるため。
			DynamicArray<Actor> tickActors = world.GetActors();
			for (const Actor& listedActor : tickActors)
			{
				Actor actor = world.GetActor(listedActor.GetEntity());
				if (!actor || !actor.Active())
				{
					continue;
				}

				Entity entity = actor.GetEntity();
				EntityID entityID = entity.GetID();

				for (Size componentIndex = 0; componentIndex < actor.ComponentIDList().size(); ++componentIndex)
				{
					ComponentID id = actor.ComponentIDList()[componentIndex];
					void* data = world.GetComponent(entityID, id);
					if (!data)
					{
						continue;
					}

					ComponentBehaviour* component = static_cast<ComponentBehaviour*>(data);
					if (component->tick_)
					{
						component->tick_(component, elapsedTime);
					}

					if (!world.GetActor(entityID))
					{
						break;
					}
				}
			}

			DynamicArray<Actor> lateTickActors = world.GetActors();
			for (const Actor& listedActor : lateTickActors)
			{
				Actor actor = world.GetActor(listedActor.GetEntity());
				if (!actor || !actor.Active())
				{
					continue;
				}

				Entity entity = actor.GetEntity();
				EntityID entityID = entity.GetID();

				for (Size componentIndex = 0; componentIndex < actor.ComponentIDList().size(); ++componentIndex)
				{
					ComponentID id = actor.ComponentIDList()[componentIndex];
					void* data = world.GetComponent(entityID, id);
					if (!data)
					{
						continue;
					}

					ComponentBehaviour* component = static_cast<ComponentBehaviour*>(data);
					if (component->lateTick_)
					{
						component->lateTick_(component, elapsedTime);
					}

					if (!world.GetActor(entityID))
					{
						break;
					}
				}
			}
		}
	}

	/**
	* [EN]
	* Advances one fixed timestep: dispatches FixedTick to every
	* ComponentBehaviour-derived component of an active actor that
	* implements it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 固定タイムステップぶん1ステップ進める: アクティブな actor が持つ、
	* FixedTick を実装している全ての ComponentBehaviour 派生コンポーネント
	* へディスパッチする。
	*/
	void SystemScheduler::Step(World& world, Float fixedTime)
	{
		/// [EN] Same copy-and-recheck walk as Run, since FixedTick may spawn or destroy actors too.
		/// [JP] Run と同じく、コピーを回して存在を確かめ直す。FixedTick も actor を生成・破棄し得るため。
		DynamicArray<Actor> fixedTickActors = world.GetActors();
		for (const Actor& listedActor : fixedTickActors)
		{
			Actor actor = world.GetActor(listedActor.GetEntity());
			if (!actor || !actor.Active())
			{
				continue;
			}

			Entity entity = actor.GetEntity();
			EntityID entityID = entity.GetID();

			for (Size componentIndex = 0; componentIndex < actor.ComponentIDList().size(); ++componentIndex)
			{
				ComponentID id = actor.ComponentIDList()[componentIndex];
				void* data = world.GetComponent(entityID, id);
				if (!data)
				{
					continue;
				}

				ComponentBehaviour* component = static_cast<ComponentBehaviour*>(data);
				if (component->fixedTick_)
				{
					component->fixedTick_(component, fixedTime);
				}

				if (!world.GetActor(entityID))
				{
					break;
				}
			}
		}
	}

	/**
	* [EN]
	* Forgets SpawnerSystem's runtime progress for every Spawner.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SpawnerSystem が持つ、全 Spawner のランタイム進行状況を忘れる。
	*/
	void SystemScheduler::Reset()
	{
		spawnerSystem_.Reset();
		lifetimeSystem_.Reset();
		commandBuffer_.Clear();
	}
}
