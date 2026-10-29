#include <FoundationEngine/World/ECS/System/SystemScheduler.h>
#include <FoundationEngine/World/ECS/System/TransformSystem.h>

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
	/**
	* [EN]
	* Walks every behaviour type for which has(hooks) is true and calls
	* call on each of its instances whose actor is active. Each type's
	* entities are copied before its walk, so an instance added during the
	* walk waits for the next one, and each instance is looked up again
	* before the call, so one removed during the walk is skipped.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* has(hooks) が真になる全てのビヘイビアの型を回し、その型のインスタンスの
	* うち、actor がアクティブなものに call を呼ぶ。各型のエンティティは回す
	* 前に写すので、途中で追加されたインスタンスは次の走査を待ち、各
	* インスタンスは呼ぶ前に引き直すので、途中で外されたものは飛ばす。
	*/
	template<typename Has, typename Call>
	void SystemScheduler::Dispatch(World& world, Has has, Call call)
	{
		SparseSetStorage& sparseSets = WorldAccessor::SparseSets(world);

		/// [EN] Behaviours always live in sparse sets, so walking the behaviour types' sets reaches every behaviour and nothing else.
		/// [JP] ビヘイビアは必ずスパースセットにあるので、ビヘイビアの型のセットを回せば、全てのビヘイビアに、そしてそれだけに届く。
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
		}
	}

	/**
	* [EN]
	* Runs one frame: (if isPlaying) runs the built-in TransformSystem so
	* actors created since the last pass have their world matrix before
	* Awake/Start, drives Awake/Start (if isPlaying), then (if
	* isPlaying) runs the structural systems (Spawner + Lifetime) through
	* SystemGraph on executor, flushes the recorded structural changes,
	* runs the built-in TransformSystem (all before Tick/LateTick so any
	* newly spawned actor's position is in this frame's world matrix), then
	* drives Tick/LateTick (if isPlaying) or EditorTick (if not). The
	* systems, Tick and LateTick advance by gameElapsedTime; EditorTick
	* advances by worldElapsedTime. Inactive actors receive none of
	* Awake/Start/Tick/LateTick/EditorTick.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1フレーム分を実行する: （isPlaying であれば）前回以降に生成された
	* actor が Awake/Start の前にワールド行列を持つよう組み込みの
	* TransformSystem を実行し、（isPlaying であれば）Awake/Start を駆動し、
	* （isPlaying であれば）構造系システム（Spawner + Lifetime）を
	* SystemGraph 経由で executor 上で実行し、記録された構造変更を flush し、
	* 組み込みの TransformSystem を実行し（すべて Tick/LateTick より前 —
	* 新しく生成された actor の位置が同じフレームのワールド行列に入るように）、
	* （isPlaying であれば）Tick/LateTick を、そうでなければ EditorTick を
	* 駆動する。システムと Tick/LateTick は gameElapsedTime で、EditorTick は
	* worldElapsedTime で進める。非アクティブな actor には
	* Awake/Start/Tick/LateTick/EditorTick のどれも送らない。
	*/
	void SystemScheduler::Run(World& world, ResourceCache& cache, JobExecutor& executor, Float worldElapsedTime, Float gameElapsedTime, Bool isPlaying)
	{
		if (isPlaying)
		{
			/// [EN] An actor created since the last TransformSystem pass (a switched-in scene, an instantiated prefab) still holds the identity world matrix,
			///      so the world matrices are built here first, letting Awake/Start read the actor's (and its parent's) actual world pose.
			/// [JP] 前回の TransformSystem 以降に生成された actor（切り替え先のシーン、Instantiate した Prefab）はワールド行列がまだ単位行列のままなので、
			///      先にここでワールド行列を作り、Awake/Start が actor（およびその親）の実際のワールドでの姿勢を読めるようにする。
			transformSystem_.Execute(world);

			/// [EN] Every behaviour is visited, with or without an OnAwake, since awoken_ must be set on all of them.
			/// [JP] OnAwake の有無に関係なく全ビヘイビアを回す。awoken_ は全てに立てる必要があるため。
			Dispatch(world, [](const ComponentHooks&) { return true; }, [](ComponentBehaviour* component)
				{
					if (!component->awoken_)
					{
						component->awoken_ = true;
						component->DispatchAwake();
					}
				});

			/// [EN] The same holds for started_, which FixedTick checks.
			/// [JP] FixedTick が確かめる started_ も同じ。
			Dispatch(world, [](const ComponentHooks&) { return true; }, [](ComponentBehaviour* component)
				{
					if (!component->started_)
					{
						component->started_ = true;
						component->DispatchStart();
					}
				});
		}

		/// [EN] Spawn before TransformSystem::Execute so a new actor's world matrix is built from its spawn Position this frame;
		///      Rigidbody::OnAwake places its body from the world matrix, not from Position.
		/// [JP] TransformSystem::Execute より前にスポーンし、新しい actor のワールド行列をこのフレームのうちにスポーン位置から作る。
		///      Rigidbody::OnAwake はボディを Position ではなくワールド行列から配置するため。
		if (isPlaying)
		{
			/// [EN] SpawnerSystem and LifetimeSystem share one CommandBuffer, so they are registered as a single serial task rather than two.
			/// [JP] SpawnerSystem と LifetimeSystem は1つの CommandBuffer を共有するので、2つではなく1つの直列タスクとして登録する。
			systemGraph_.Clear();
			systemGraph_.Add(Query<Read<Spawner>, Read<Lifetime>>::ReadSignature(), Query<Read<Spawner>, Read<Lifetime>>::WriteSignature(), [this, &world, gameElapsedTime]()
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
			Dispatch(world, [](const ComponentHooks& hooks) { return hooks.tick_ != nullptr; }, [gameElapsedTime](ComponentBehaviour* component)
				{
					component->DispatchTick(gameElapsedTime);
				});

			Dispatch(world, [](const ComponentHooks& hooks) { return hooks.lateTick_ != nullptr; }, [gameElapsedTime](ComponentBehaviour* component)
				{
					component->DispatchLateTick(gameElapsedTime);
				});
		}
		else
		{
			/// [EN] While not playing, EditorTick takes Tick's place, passing the world's elapsed time, since the game's stays 0 while not playing.
			/// [JP] プレイしていない間は Tick の代わりに EditorTick を送る。ゲームの経過時間は 0 のままなので、ワールドの経過時間を渡す。
			Dispatch(world, [](const ComponentHooks& hooks) { return hooks.editorTick_ != nullptr; }, [worldElapsedTime](ComponentBehaviour* component)
				{
					component->DispatchEditorTick(worldElapsedTime);
				});
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
		/// [EN] FixedTick reaches only a component that has been through Start, so a fixed step never sees a component before Awake has set it up (a Rigidbody's body, for one).
		/// [JP] FixedTick は Start を通ったコンポーネントにだけ届く。固定ステップが、Awake で準備される前(Rigidbody のボディなど)のコンポーネントに触れないようにするため。
		Dispatch(world, [](const ComponentHooks& hooks) { return hooks.fixedTick_ != nullptr; }, [fixedTime](ComponentBehaviour* component)
			{
				if (component->started_)
				{
					component->DispatchFixedTick(fixedTime);
				}
			});
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
