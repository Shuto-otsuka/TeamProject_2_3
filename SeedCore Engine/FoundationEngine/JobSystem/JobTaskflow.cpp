#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs an empty taskflow with the given display name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定された表示名で、空のタスクフローを構築する。
	*/
	JobTaskflow::JobTaskflow(const String& name) :FlowBuilder(graph_), name_(name)
	{
		/// No Code
	}

	/**
	* [EN]
	* Constructs an empty taskflow with no name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 名前を持たない、空のタスクフローを構築する。
	*/
	JobTaskflow::JobTaskflow() :FlowBuilder(graph_)
	{
		/// No Code
	}

	/**
	* [EN]
	* Move-constructs, transferring rhs's graph, name, and pending topologies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* rhs のグラフ、名前、保留中のトポロジーを移譲してムーブ構築する。
	*/
	JobTaskflow::JobTaskflow(JobTaskflow&& rhs) :FlowBuilder(graph_)
	{
		/// [EN] rhs is locked while its members are taken, since another thread may be queueing a run on it.
		/// [JP] rhs のメンバーを取る間はロックする。別のスレッドが実行を積んでいる可能性があるため。
		std::scoped_lock<std::mutex> lock(rhs.mutex_);
		name_ = std::move(rhs.name_);
		graph_ = std::move(rhs.graph_);
		topologies_ = std::move(rhs.topologies_);
	}

	/**
	* [EN]
	* Move-assigns, transferring rhs's graph, name, and pending topologies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* rhs のグラフ、名前、保留中のトポロジーを移譲してムーブ代入する。
	*/
	JobTaskflow& JobTaskflow::operator=(JobTaskflow&& rhs)
	{
		if (this != &rhs)
		{
			/// [EN] Both sides are locked at once; std::scoped_lock picks an order that cannot deadlock.
			/// [JP] 両側を同時にロックする。std::scoped_lock は、デッドロックしない順序で取る。
			std::scoped_lock<std::mutex, std::mutex> lock(mutex_, rhs.mutex_);
			name_ = std::move(rhs.name_);
			graph_ = std::move(rhs.graph_);
			topologies_ = std::move(rhs.topologies_);
		}
		return *this;
	}

	/**
	* [EN]
	* Returns the number of tasks (nodes) currently in the graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフ内に現在存在するタスク（ノード）の数を返す。
	*/
	Size JobTaskflow::NumberTasks()const
	{
		return graph_.size();
	}

	/**
	* [EN]
	* Returns whether the graph currently has no tasks.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフが現在タスクを持たないかどうかを返す。
	*/
	Bool JobTaskflow::Empty()const
	{
		return graph_.empty();
	}

	/**
	* [EN]
	* Sets the taskflow's display name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タスクフローの表示名を設定する。
	*/
	void JobTaskflow::Name(const String& name)
	{
		name_ = name;
	}

	/**
	* [EN]
	* Returns the taskflow's display name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タスクフローの表示名を返す。
	*/
	const String& JobTaskflow::Name()const
	{
		return name_;
	}

	/**
	* [EN]
	* Removes every task from the graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフからすべてのタスクを削除する。
	*/
	void JobTaskflow::Clear()
	{
		/// [EN] Every node is released, so JobTask handles taken from this taskflow must not be used afterwards.
		/// [JP] 全ノードが解放されるので、このタスクフローから得た JobTask ハンドルはこの後使ってはならない。
		graph_.clear();
	}

	/**
	* [EN]
	* Removes every edge from from to to, on both nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* from から to へのエッジを、両方のノードから全て取り除く。
	*/
	void JobTaskflow::RemoveDependency(JobTask from, JobTask to)
	{
		/// [EN] Each edge is recorded on both nodes, so it is removed from from's successors and from to's predecessors.
		/// [JP] エッジは両方のノードに記録されているので、from の後続と to の先行ノードの両方から取り除く。
		from.node_->RemoveSuccessors(to.node_);

		to.node_->RemovePredecessors(from.node_);
	}

	/**
	* [EN]
	* Returns a reference to the underlying JobGraph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部の JobGraph への参照を返す。
	*/
	JobGraph& JobTaskflow::Graph()
	{
		return graph_;
	}

	/**
	* [EN]
	* Appends a run to the queue and returns the queue size from before
	* the append; 0 means no other run is in progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 実行を列の末尾へ足し、足す前の列の長さを返す。0 は他に進行中の
	* 実行が無いことを意味する。
	*/
	Size JobTaskflow::FetchEnqueue(ResourceRef<JobTopology> topologies)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		/// [EN] The size before insertion tells the caller whether this run is the only one and must be started now.
		/// [JP] 挿入前の長さで、呼び出し側はこの実行が唯一のもので、今すぐ始めるべきかを知る。
		auto preSize = topologies_.size();
		topologies_.emplace(std::move(topologies));
		return preSize;
	}
}
