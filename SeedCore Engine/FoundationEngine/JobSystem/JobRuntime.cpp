#include <FoundationEngine/JobSystem/JobRuntime.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>

namespace SeedCore
{
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
	JobPreemptiveRuntime::JobPreemptiveRuntime(JobExecutor& executor, JobWorker& worker, JobNode* node) : executor_(executor), worker_(worker), node_(node)
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns a reference to the executor running this task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクを実行しているエグゼキュータへの参照を返す。
	*/
	JobExecutor& JobPreemptiveRuntime::Executor()
	{
		return executor_;
	}

	/**
	* [EN]
	* Returns a reference to the worker thread currently executing this task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクを現在実行しているワーカースレッドへの参照を返す。
	*/
	JobWorker& JobPreemptiveRuntime::Worker()
	{
		return worker_;
	}

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
	void JobPreemptiveRuntime::Schedule(JobTask task)
	{
		auto node = task.node_;

		/// [EN] A zero counter means the task no longer waits on any predecessor when it runs.
		/// [JP] カウンタを 0 にすることで、実行時にどの先行ノードも待たなくなる。
		node->joinCounter_.store(0, std::memory_order_relaxed);

		/// [EN] Counted on the parent (or topology) before scheduling, so the run cannot be seen as finished while the task is in flight.
		/// [JP] スケジュールの前に親（あるいはトポロジー）の側で数えておき、タスクの実行中に実行全体が終わったと見なされないようにする。
		auto& join = node->parent_ ? node->parent_->joinCounter_ : node->topology_->joinCounter_;
		join.fetch_add(1, std::memory_order_relaxed);
		executor_.Schedule(worker_, node);
	}

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
	void JobPreemptiveRuntime::Corun()
	{
		{
			/// [EN] The node is the blocking point while coruning, so exceptions from its children are stored on it.
			/// [JP] Corun の間はこのノードが待つ地点なので、子からの例外はこのノードに格納される。
			JobExplicitAnchorGuard anchor(node_);

			/// [EN] The target is 1, not 0: InvokeRuntimeTaskImplementation adds one count to the node before calling the callable.
			/// [JP] 目標は 0 ではなく 1。InvokeRuntimeTaskImplementation が、処理を呼ぶ前にノードへカウントを1つ足しているため。
			executor_.CorunUntil(worker_, [this]()->Bool {return node_->joinCounter_.load(std::memory_order_acquire) == 1;});
		}

		/// [EN] An exception thrown by a child surfaces here, inside the runtime task that waited for it.
		/// [JP] 子が投げた例外は、ここで、それを待っていたランタイムタスクの中へ出てくる。
		node_->RethrowException();
	}

	/**
	* [EN]
	* Waits for this task's children in the same way as Corun.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Corun と同じ方法で、このタスクの子を待つ。
	*/
	void JobPreemptiveRuntime::CorunAll()
	{
		Corun();
	}

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
	Bool JobPreemptiveRuntime::Cancelled()
	{
		/// [EN] The same check the executor uses to skip nodes of a cancelled or failed run.
		/// [JP] エグゼキュータが、キャンセルや失敗した実行のノードを飛ばすときと同じ確認。
		return node_->ParentCancelled();
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a runtime handle running under executor/worker.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* executor/worker のもとで実行されるランタイムハンドルを構築する。
	*/
	JobNonpreemptiveRuntime::JobNonpreemptiveRuntime(JobExecutor& executor, JobWorker& worker) :executor_(executor), worker_(worker)
	{
		/// No Code
	}

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
	void JobNonpreemptiveRuntime::Schedule(JobTask task)
	{
		auto node = task.node_;

		/// [EN] A zero counter means the task no longer waits on any predecessor when it runs.
		/// [JP] カウンタを 0 にすることで、実行時にどの先行ノードも待たなくなる。
		node->joinCounter_.store(0, std::memory_order_relaxed);

		/// [EN] Counted on the parent (or topology) before scheduling, so the run cannot be seen as finished while the task is in flight.
		/// [JP] スケジュールの前に親（あるいはトポロジー）の側で数えておき、タスクの実行中に実行全体が終わったと見なされないようにする。
		auto& join = node->parent_ ? node->parent_->joinCounter_ : node->topology_->joinCounter_;
		join.fetch_add(1, std::memory_order_relaxed);
		executor_.Schedule(worker_, node);
	}
}
