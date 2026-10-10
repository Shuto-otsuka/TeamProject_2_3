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
	* world matrix), then drives Tick/LateTick (if isPlaying) or
	* EditorTick (if not). The systems, Tick and LateTick advance by
	* gameElapsedTime; EditorTick advances by worldElapsedTime. Inactive
	* actors receive none of Awake/Start/Tick/LateTick/EditorTick.
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
	* （isPlaying であれば）Tick/LateTick を、そうでなければ EditorTick を
	* 駆動する。システムと Tick/LateTick は gameElapsedTime で、EditorTick は
	* worldElapsedTime で進める。非アクティブな actor には
	* Awake/Start/Tick/LateTick/EditorTick のどれも送らない。
	*/
	void SystemScheduler::Run(World& world, ResourceCache& cache, JobExecutor& executor, Float worldElapsedTime, Float gameElapsedTime, Bool isPlaying)
	{
		if (isPlaying)
		{
			/// [EN] Each pass walks the actors that existed when it began; actors spawned during a pass are first processed in the next frame.
			/// [JP] 各パスは、開始時点で存在していた actor を回す。パス中に生成された actor は次のフレームから処理される。
			DynamicArray<Actor> awakeActors = world.GetActors();
			for (const Actor& listedActor : awakeActors)
			{
				/// [EN] An actor destroyed earlier in the pass is skipped.
				/// [JP] パスの途中で破棄された actor は飛ばす。
				Actor actor = world.GetActor(listedActor.GetEntity());
				if (!actor || !actor.Active())
				{
					continue;
				}

				Entity entity = actor.GetEntity();
				EntityID entityID = entity.GetID();

				/// [EN] The component list is read by index on every step, so it always reflects the actor's current components.
				/// [JP] コンポーネント一覧は毎回インデックスで読み、常にその actor の現在のコンポーネントを反映させる。
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

					/// [EN] Processing of an actor ends once it no longer exists.
					/// [JP] actor が存在しなくなった時点で、その actor の処理を終える。
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
				[this, &world, gameElapsedTime]()
				{
					moveSystem_.Execute(world, gameElapsedTime);
				});
			systemGraph_.Add(
				Query<Read<Spawner>, Read<Lifetime>>::GetReadSignature(),
				Query<Read<Spawner>, Read<Lifetime>>::GetWriteSignature(),
				[this, &world, gameElapsedTime]()
				{
					spawnerSystem_.Execute(commandBuffer_, world, gameElapsedTime);
					lifetimeSystem_.Execute(commandBuffer_, world, gameElapsedTime);
				});
			systemGraph_.Run(executor);
		}

		/// [EN] Flush the structural changes systems recorded (spawns, destroys) before TransformSystem, so this frame's world matrices already reflect them.
		/// [JP] システムが記録した構造変更(スポーン・破棄)を TransformSystem より前に flush し、今フレームのワールド行列に反映させる。
		commandBuffer_.Flush(world, cache);

		transformSystem_.Execute(world);

		if (isPlaying)
		{
			/// [EN] Tick and LateTick walk the actors the same way as Awake/Start above.
			/// [JP] Tick と LateTick も、上の Awake/Start と同じ方法で actor を回す。
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
						component->tick_(component, gameElapsedTime);
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
						component->lateTick_(component, gameElapsedTime);
					}

					if (!world.GetActor(entityID))
					{
						break;
					}
				}
			}
		}
		else
		{
			/// [EN] While not playing, EditorTick takes Tick's place and walks the actors the same way, passing the world's elapsed time, since the game's stays 0 while not playing.
			/// [JP] プレイしていない間は Tick の代わりに EditorTick を、同じ方法で actor を回して送る。ゲームの経過時間は 0 のままなので、ワールドの経過時間を渡す。
			DynamicArray<Actor> editorTickActors = world.GetActors();
			for (const Actor& listedActor : editorTickActors)
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
					if (component->editorTick_)
					{
						component->editorTick_(component, worldElapsedTime);
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
	* implements it and has already been through Start.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 固定タイムステップぶん1ステップ進める: アクティブな actor が持つ、
	* FixedTick を実装していて既に Start を通った全ての ComponentBehaviour
	* 派生コンポーネントへディスパッチする。
	*/
	void SystemScheduler::Step(World& world, Float fixedTime)
	{
		/// [EN] FixedTick walks the actors the same way as Run: those that existed when the step began, skipping any destroyed along the way.
		/// [JP] FixedTick も Run と同じ方法で actor を回す。ステップ開始時点で存在していたものを、途中で破棄されたものを飛ばしながら処理する。
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

				/// [EN] FixedTick reaches only a component that has been through Start, so a fixed step never sees a component before Awake has set it up (a Rigidbody's body, for one).
				/// [JP] FixedTick は Start を通ったコンポーネントにだけ届く。固定ステップが、Awake で準備される前(Rigidbody のボディなど)のコンポーネントに触れないようにするため。
				ComponentBehaviour* component = static_cast<ComponentBehaviour*>(data);
				if (component->fixedTick_ && component->started_)
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
