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

	/**
	* [EN]
	* Drives the built-in per-frame simulation step: dispatches the
	* ComponentBehaviour lifecycle callbacks (Awake/Start via Run, Tick/
	* LateTick via Run, FixedTick via Step) and runs the structural
	* systems (Spawner, Lifetime) through a SystemGraph - so
	* non-conflicting ones run in parallel on a JobExecutor - then the
	* built-in TransformSystem. Callbacks are dispatched behaviour type by
	* behaviour type, walking each type's sparse set, so the cost follows
	* how many behaviours exist rather than how many actors do. FixedTick
	* is dispatched separately, through Step, since it runs at a fixed
	* timestep and may fire zero or several times per rendered frame
	* rather than exactly once like Run.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 組み込みの毎フレームシミュレーションステップを駆動するクラス:
	* ComponentBehaviour のライフサイクルコールバック（Awake/Start は Run、
	* Tick/LateTick も Run、FixedTick は Step）をディスパッチし、構造系
	* システム（Spawner、Lifetime）を SystemGraph 経由で実行し（衝突しない
	* ものは JobExecutor 上で並列に走る）、続けて組み込みの TransformSystem
	* を実行する。コールバックはビヘイビアの型ごとに、その型のスパース
	* セットを回して送るので、費用は actor の数ではなくビヘイビアの数に
	* 比例する。FixedTick は固定タイムステップで動作し、Run のように毎フレーム
	* 必ず1回ではなく0回や複数回発火し得るため、Step として別に配線する。
	*/
	class SEEDCORE_API SystemScheduler
	{
	public:
		/**
		* [EN]
		* Default constructor: starts with no registered systems.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: 登録済みシステムが無い状態から開始する。
		*/
		SystemScheduler() = default;

		/**
		* [EN]
		* Destructor; uses the compiler-generated default.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デストラクタ。コンパイラ生成のデフォルトを使用する。
		*/
		~SystemScheduler() = default;

		/**
		* [EN]
		* Runs one frame: drives Awake/Start (if isPlaying), then (if
		* isPlaying) runs the structural systems (Spawner + Lifetime)
		* through a SystemGraph on executor, flushes the recorded
		* structural changes, and runs the built-in TransformSystem (all
		* before Tick/LateTick so any newly spawned actor's position is in
		* this frame's world matrix), then
		* drives Tick/LateTick (if isPlaying) or EditorTick (if not). The
		* systems, Tick and LateTick advance by gameElapsedTime; EditorTick
		* advances by worldElapsedTime, since the game's time stays 0 while
		* not playing. Awake/Start/Tick/LateTick/EditorTick are only
		* dispatched to active actors, so an actor that starts inactive
		* receives Awake/Start the first frame it is active.
		* Shared by Runtime and Editor, so cache and executor are threaded
		* through explicitly rather than assumed global.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1フレーム分を実行する: （isPlaying であれば）Awake/Start を駆動し、
		* （isPlaying であれば）構造系システム（Spawner + Lifetime）を
		* SystemGraph 経由で executor 上で実行し、記録された構造変更を flush
		* し、組み込みの TransformSystem を実行する（すべて Tick/LateTick より
		* 前 — 新しく生成された actor の位置が同じフレームのワールド行列に
		* 入るように）。その後（isPlaying であれば）Tick/LateTick を、そうで
		* なければ EditorTick を駆動する。システムと Tick/LateTick は
		* gameElapsedTime で、EditorTick は worldElapsedTime で進める（プレイして
		* いない間はゲームの時間が 0 のままのため）。
		* Awake/Start/Tick/LateTick/EditorTick はアクティブな actor にだけ送るので、
		* 非アクティブで始まった actor は、アクティブになった最初の
		* フレームで Awake/Start を受け取る。
		* Runtime と Editor の両方から使われるため、cache と executor は
		* グローバル前提にせず明示的に受け渡す。
		*/
		void Run(World& world, ResourceCache& cache, JobExecutor& executor, Float worldElapsedTime, Float gameElapsedTime, Bool isPlaying = true);

		/**
		* [EN]
		* Advances one fixed timestep: dispatches FixedTick to every
		* ComponentBehaviour-derived component of an active actor that
		* implements it and has already been through Start, so FixedTick
		* always follows Awake/Start, as Tick does. The caller
		* is expected to invoke this once per fixed timestep (e.g. from an
		* accumulator loop, alongside stepping the physics simulation by
		* the same fixedTime), independently of how many times Run fires
		* per rendered frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 固定タイムステップぶん1ステップ進める: アクティブな actor が持つ、
		* FixedTick を実装していて既に Start を通った全ての ComponentBehaviour
		* 派生コンポーネントへディスパッチする。そのため FixedTick は、Tick と
		* 同じく必ず Awake/Start の後になる。
		* 呼び出し側は、Run が1フレームに何回発火するかとは無関係に、
		* 固定タイムステップごとに（例えばアキュムレータループの中で、
		* 同じ fixedTime 分だけ物理シミュレーションをステップするのと
		* あわせて）1回ずつ呼び出すことを想定している。
		*/
		void Step(World& world, Float fixedTime);

		/**
		* [EN]
		* Forgets SpawnerSystem's runtime progress for every Spawner -
		* call this whenever the World was reset out from under this
		* scheduler (e.g. WorldSnapshot::Restore on Editor Stop), so
		* stale EntityIDs from the previous session aren't carried
		* forward into the next Play.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SpawnerSystem が持つ、全 Spawner のランタイム進行状況を忘れる -
		* この scheduler の下で World がリセットされた時(例: Editor の
		* Stop 時の WorldSnapshot::Restore)に呼ぶこと。前回セッションの
		* 古い EntityID が次の Play へ持ち越されないようにするため。
		*/
		void Reset();

	private:
		/**
		* [EN]
		* Walks every behaviour type for which has(hooks) is true and calls
		* call on each of its instances whose actor is active. Each type's
		* entities are copied before its walk, so an instance added during
		* the walk waits for the next one, and each instance is looked up
		* again before the call, so one removed during the walk is skipped.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* has(hooks) が真になる全てのビヘイビアの型を回し、その型のインスタンス
		* のうち、actor がアクティブなものに call を呼ぶ。各型のエンティティは
		* 回す前に写すので、途中で追加されたインスタンスは次の走査を待ち、
		* 各インスタンスは呼ぶ前に引き直すので、途中で外されたものは飛ばす。
		*/
		template<typename Has, typename Call>
		void Dispatch(World& world, Has has, Call call);

	private:
		/// [EN] Built-in system that recomputes world-space transform matrices from local transforms and the parent hierarchy.
		/// [JP] ローカルトランスフォームと親階層から、ワールド空間変換行列を再計算する組み込みシステム。
		TransformSystem transformSystem_;

		/// [EN] Built-in system that drives every Spawner component's periodic prefab instantiation.
		/// [JP] 全 Spawner コンポーネントの周期的なプレハブ生成を駆動する組み込みシステム。
		SpawnerSystem spawnerSystem_;

		/// [EN] Built-in system that counts down every Lifetime component and records its owning actor's destruction once it hits zero.
		/// [JP] 全 Lifetime コンポーネントをカウントダウンし、0 に達した時点で所有アクターの破棄を記録する組み込みシステム。
		LifetimeSystem lifetimeSystem_;

		/// [EN] Collects the structural World changes SpawnerSystem/LifetimeSystem record each frame; flushed between the system-update block and TransformSystem.
		/// [JP] SpawnerSystem/LifetimeSystem が毎フレーム記録する World の構造変更を集める。システム更新ブロックと TransformSystem の間で flush される。
		CommandBuffer commandBuffer_;

		/// [EN] Schedules the pre-TransformSystem systems by their component access, running non-conflicting ones in parallel on the frame's JobExecutor; rebuilt each Run.
		/// [JP] TransformSystem 前のシステムをコンポーネントアクセスに基づいてスケジュールし、衝突しないものをそのフレームの JobExecutor 上で並列に走らせる。Run ごとに再構築する。
		SystemGraph systemGraph_;

		/// [EN] The entities of the behaviour type being dispatched, copied before its walk; kept between walks so its memory is reused.
		/// [JP] ディスパッチ中のビヘイビアの型のエンティティ。回す前に写す。走査をまたいで持ち、メモリを使い回す。
		DynamicArray<EntityID> entities_;
	};
}
