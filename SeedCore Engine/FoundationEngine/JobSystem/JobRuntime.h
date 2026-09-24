#pragma once
#include <FoundationEngine/JobSystem/JobTask.h>

namespace SeedCore
{
	/**
	* [EN]
	* Handle passed to a PreemptiveRuntime task's callable, letting it
	* interact with the scheduler while running: schedule extra tasks,
	* wait for them while helping with other work (Corun/CorunAll),
	* and ask whether the run has been cancelled. Tasks still running
	* when the callable returns keep the node suspended until they
	* finish.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* PreemptiveRuntime タスクの呼び出し可能オブジェクトへ渡される
	* ハンドル。実行中にスケジューラと連携できるようにする。追加タスクの
	* スケジューリング、他の処理を手伝いながらそれらを待つこと
	* （Corun/CorunAll）、実行がキャンセルされたかの確認ができる。処理が
	* 戻った時点でまだ走っているタスクがあれば、それらが終わるまで
	* ノードは中断したままになる。
	*/
	class JobPreemptiveRuntime
	{
	private:
		/// [EN] Only the executor creates a runtime, right before calling the task's callable.
		/// [JP] ランタイムを作るのはエグゼキュータだけで、タスクの処理を呼ぶ直前に作る。
		friend class JobExecutor;
		friend class FlowBuilder;

	public:
		/**
		* [EN]
		* Returns a reference to the executor running this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクを実行しているエグゼキュータへの参照を返す。
		*/
		JobExecutor& Executor();

		/**
		* [EN]
		* Returns a reference to the worker thread currently executing this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクを現在実行しているワーカースレッドへの参照を返す。
		*/
		JobWorker& Worker();

		/**
		* [EN]
		* Schedules task to run right away, ignoring its predecessors,
		* and counts it as an outstanding child of its parent node (or of
		* its topology when it has no parent).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* task を、先行ノードを無視してすぐ実行されるようスケジュールし、
		* その親ノード（親が無ければトポロジー）の未完了の子として数える。
		*/
		void Schedule(JobTask task);

		/**
		* [EN]
		* Keeps the current worker running other tasks until every child
		* this task has spawned has finished, so the task can wait for
		* them without leaving its thread idle. An exception thrown by a
		* child is rethrown here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクが生成した子が全て終わるまで、今のワーカーに他のタスク
		* を実行させ続ける。タスクはスレッドを遊ばせずに子を待てる。子が
		* 投げた例外はここで投げ直される。
		*/
		void Corun();

		/**
		* [EN]
		* Waits for this task's children in the same way as Corun.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Corun と同じ方法で、このタスクの子を待つ。
		*/
		void CorunAll();

		/**
		* [EN]
		* Returns whether this task's run has been cancelled or has failed
		* with an exception.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクの実行がキャンセルされたか、例外で失敗しているかを返す。
		*/
		Bool Cancelled();

	private:
		/**
		* [EN]
		* Constructs a runtime handle bound to node, running under executor/worker.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* executor/worker のもとで実行される、node に紐づいたランタイム
		* ハンドルを構築する。
		*/
		explicit JobPreemptiveRuntime(JobExecutor& executor, JobWorker& worker, JobNode* node);

		/// [EN] The executor running this task.
		/// [JP] このタスクを実行しているエグゼキュータ。
		JobExecutor& executor_;

		/// [EN] The worker thread currently executing this task; scheduled tasks go onto its queue.
		/// [JP] このタスクを現在実行しているワーカースレッド。スケジュールしたタスクはこのキューへ積まれる。
		JobWorker& worker_;

		/// [EN] The node running this task; its join counter tracks the children spawned through this runtime.
		/// [JP] このタスクを実行しているノード。その join カウンタが、このランタイムを通じて生成した子を数える。
		JobNode* node_;
	};

	/**
	* [EN]
	* Handle passed to a NonpreemptiveRuntime task's callable, letting it
	* schedule extra tasks. Unlike JobPreemptiveRuntime, the node never
	* suspends: it finishes as soon as the callable returns, without
	* waiting for what it scheduled.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* NonpreemptiveRuntime タスクの呼び出し可能オブジェクトへ渡される
	* ハンドル。追加タスクをスケジュールできる。JobPreemptiveRuntime と
	* 異なりノードは決して中断せず、スケジュールしたものを待たずに、
	* 処理が戻った時点で終わる。
	*/
	class JobNonpreemptiveRuntime
	{
	private:
		/// [EN] Only the executor creates a runtime, right before calling the task's callable.
		/// [JP] ランタイムを作るのはエグゼキュータだけで、タスクの処理を呼ぶ直前に作る。
		friend class JobExecutor;

	public:
		/**
		* [EN]
		* Schedules task to run right away, ignoring its predecessors,
		* and counts it as an outstanding child of its parent node (or of
		* its topology when it has no parent).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* task を、先行ノードを無視してすぐ実行されるようスケジュールし、
		* その親ノード（親が無ければトポロジー）の未完了の子として数える。
		*/
		void Schedule(JobTask task);

	private:
		/**
		* [EN]
		* Constructs a runtime handle running under executor/worker.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* executor/worker のもとで実行されるランタイムハンドルを構築する。
		*/
		explicit JobNonpreemptiveRuntime(JobExecutor& executor, JobWorker& worker);

		/// [EN] The executor running this task.
		/// [JP] このタスクを実行しているエグゼキュータ。
		JobExecutor& executor_;

		/// [EN] The worker thread currently executing this task.
		/// [JP] このタスクを現在実行しているワーカースレッド。
		JobWorker& worker_;
	};
}
