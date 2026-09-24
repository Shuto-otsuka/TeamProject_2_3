#include <FoundationEngine/JobSystem/FlowBuilder.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a builder that creates/modifies tasks in graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* graph 内のタスクを生成・変更するビルダーを構築する。
	*/
	FlowBuilder::FlowBuilder(JobGraph& graph) :graph_(graph)
	{
		/// No Code
	}

	/**
	* [EN]
	* Removes task from the graph, first detaching it from every node
	* it is connected to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* task をグラフから削除する。先に、つながっている全ノードから task
	* を切り離す。
	*/
	void FlowBuilder::Erase(JobTask task)
	{
		/// [EN] An empty handle refers to no node, so there is nothing to remove.
		/// [JP] 空のハンドルはどのノードも指していないので、消すものは無い。
		if (!task.node_)
		{
			return;
		}

		/// [EN] For each successor of task (front part of task's edges_), drop task from that node's predecessors.
		/// [JP] task の後続（task の edges_ の前方）それぞれについて、そのノードの先行ノードから task を外す。
		for (Size index = 0;index < task.node_->numberSuccessors_;++index)
		{
			task.node_->edges_[index]->RemovePredecessors(task.node_);
		}

		/// [EN] For each predecessor of task (rest of task's edges_), drop task from that node's successors.
		/// [JP] task の先行ノード（task の edges_ の残り）それぞれについて、そのノードの後続から task を外す。
		for (Size index = task.node_->numberSuccessors_;index < task.node_->edges_.size();++index)
		{
			task.node_->edges_[index]->RemoveSuccessors(task.node_);
		}

		/// [EN] With no edge left pointing at it, the node can be released safely.
		/// [JP] 指しているエッジが無くなったので、ノードを安全に解放できる。
		graph_.erase(task.node_);
	}

	/**
	* [EN]
	* Creates a new module task that takes ownership of graph (moved in)
	* and returns a handle to it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* graph の所有権を（ムーブで）引き受ける新しいモジュールタスクを
	* 生成し、そのハンドルを返す。
	*/
	JobTask FlowBuilder::Adopt(JobGraph&& graph)
	{
		/// [EN] The graph is moved into the node, so it lives exactly as long as the task does.
		/// [JP] グラフはノードへムーブされるので、タスクとちょうど同じ期間だけ生きる。
		return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::AdoptedModule>{}, std::move(graph)));
	}

	/**
	* [EN]
	* Creates a new task with no assigned work (a placeholder) and
	* returns a handle to it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 処理が割り当てられていない新しいタスク（プレースホルダー）を
	* 生成し、そのハンドルを返す。
	*/
	JobTask FlowBuilder::Placeholder()
	{
		/// [EN] The node gets no work; it can be given some later, or serve only as a join point for dependencies.
		/// [JP] ノードには処理を持たせない。後から与えるか、依存関係の合流点としてだけ使う。
		auto node = graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::Placeholder>{});
		return JobTask(node);
	}

	/**
	* [EN]
	* Chains every task in keys into a straight-line sequence,
	* establishing a precedence edge between each consecutive pair.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* keys の各タスクを直線的な順序に連結し、連続する各ペアの間に
	* 先行関係エッジを設定する。
	*/
	void FlowBuilder::Linearize(DynamicArray<JobTask>& keys)
	{
		this->Linearize<DynamicArray<JobTask>>(keys);
	}

	/**
	* [EN]
	* Chains every task in keys into a straight-line sequence, from an
	* initializer list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* keys の各タスクを直線的な順序に連結する。初期化子リストから受け取る。
	*/
	void FlowBuilder::Linearize(std::initializer_list<JobTask>& keys)
	{
		this->Linearize<std::initializer_list<JobTask>>(keys);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a subflow builder for node's subgraph, running under
	* executor/worker, and clears the graph as a fresh start for this run.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* executor/worker のもとで実行される、node のサブグラフに対する
	* サブフロービルダーを構築し、この実行のための新規状態として
	* graph をクリアする。
	*/
	JobSubflow::JobSubflow(JobExecutor& executor, JobWorker& worker, JobNode* node, JobGraph& graph) :FlowBuilder(graph), executor_(executor), worker_(worker), node_(node)
	{
		/// [EN] Clears the joined/retain flags left over from a previous run of this node's subflow.
		/// [JP] このノードのサブフローの前回実行から残っている、合流済み/保持のフラグを消す。
		node_->nstate_ &= ~(JobNodeState::JOINED_SUBFLOW | JobNodeState::RETAIN_SUBFLOW);

		/// [EN] The subgraph starts empty on every run, so it is built from scratch each time.
		/// [JP] サブグラフは実行のたびに空から始まり、毎回作り直される。
		graph.clear();
	}

	/**
	* [EN]
	* Runs this subflow's subgraph to completion on the current worker
	* and marks the subflow as joined. Throws if it was already joined.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このサブフローのサブグラフを、今のワーカー上で完了まで実行し、
	* 合流済みにする。既に合流済みなら例外を投げる。
	*/
	void JobSubflow::Join()
	{
		/// [EN] Joining twice would run the same subgraph a second time within one run.
		/// [JP] 2回合流すると、1回の実行の中で同じサブグラフをもう一度回すことになる。
		if (!Joinable())
		{
			SC_THROW("サブフローは既に合流済みです。");
		}

		/// [EN] Synchronously run the subgraph to completion on the calling worker, helping process other work while waiting.
		/// [JP] 呼び出し元のワーカー上でサブグラフを同期的に完了まで実行する。待機中は他の処理を手伝う。
		executor_.CorunGraph(worker_, graph_, node_->topology_, node_);

		/// [EN] Marked as joined so the executor does not schedule the subgraph again after the task returns.
		/// [JP] 合流済みにしておくことで、タスクが戻った後にエグゼキュータがサブグラフを再びスケジュールしない。
		node_->nstate_ |= JobNodeState::JOINED_SUBFLOW;
	}

	/**
	* [EN]
	* Returns whether this subflow's subgraph is currently joinable (has
	* not already been joined).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このサブフローのサブグラフが現在 join 可能か（まだ join されて
	* いないか）を返す。
	*/
	Bool JobSubflow::Joinable()const noexcept
	{
		return !(node_->nstate_ & JobNodeState::JOINED_SUBFLOW);
	}

	/**
	* [EN]
	* Returns a reference to the JobExecutor running this subflow.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このサブフローを実行している JobExecutor への参照を返す。
	*/
	JobExecutor& JobSubflow::Executor()noexcept
	{
		return executor_;
	}

	/**
	* [EN]
	* Returns a reference to this subflow's underlying graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このサブフローの内部グラフへの参照を返す。
	*/
	JobGraph& JobSubflow::Graph()
	{
		return graph_;
	}

	/**
	* [EN]
	* Sets whether this subflow's subgraph should be retained instead of
	* being cleared after it finishes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このサブフローのサブグラフを、完了後にクリアするのではなく
	* 保持するかどうかを設定する。
	*/
	void JobSubflow::Retain(Bool flag)noexcept
	{
		if (flag)
		{
			node_->nstate_ |= JobNodeState::RETAIN_SUBFLOW;
		}
		else
		{
			node_->nstate_ &= ~JobNodeState::RETAIN_SUBFLOW;
		}
	}

	/**
	* [EN]
	* Returns whether this subflow's subgraph is currently set to be retained.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このサブフローのサブグラフが現在保持される設定になっているかを
	* 返す。
	*/
	Bool JobSubflow::Retain()const
	{
		return node_->nstate_ & JobNodeState::RETAIN_SUBFLOW;
	}
}
