#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/JobSystem/JobDeclaretions.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobTask.h>

namespace SeedCore
{
	/**
	* [EN]
	* Public graph-construction API for a JobGraph: creates tasks
	* (dispatching to the right JobNode::NodeHandle alternative based on
	* the callable's signature), wires their dependencies, and manages
	* removal/linearization. JobTaskflow derives from this to expose
	* graph building; JobSubflow also derives from it to let a task
	* build its own nested subgraph at runtime.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JobGraph に対する、公開されたグラフ構築 API。呼び出し可能
	* オブジェクトのシグネチャに応じて適切な JobNode::NodeHandle の
	* 選択肢へディスパッチしながらタスクを生成し、それらの依存関係を
	* 配線し、削除・直列化を管理する。JobTaskflow はこれを継承して
	* グラフ構築機能を公開する。JobSubflow も同様にこれを継承し、
	* タスクが実行時に自身のネストされたサブグラフを構築できるようにする。
	*/
	class FlowBuilder
	{
	private:
		/// [EN] The executor reads graph_ when it schedules what a builder produced.
		/// [JP] エグゼキュータは、ビルダーが作ったものをスケジュールするときに graph_ を読む。
		friend class JobExecutor;

	public:
		/**
		* [EN]
		* Constructs a builder that creates/modifies tasks in graph.
		* The builder does not own graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* graph 内のタスクを生成・変更するビルダーを構築する。ビルダーは
		* graph を所有しない。
		*/
		FlowBuilder(JobGraph& graph);

		/**
		* [EN]
		* Creates a new Static task from callable (a parameterless,
		* void-returning callable) and returns a handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable（引数なしで void を返す呼び出し可能オブジェクト）から
		* 新しい Static タスクを生成し、そのハンドルを返す。
		*/
		template<StaticTaskLike C>
		JobTask emplace(C&& callable)
		{
			/// [EN] A new node starts with no state, no topology and no parent; those are filled in when a run is prepared.
			/// [JP] 新しいノードは状態・トポロジー・親を持たずに始まる。それらは実行の準備のときに埋まる。
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::Static>{}, std::forward<C>(callable)));
		}

		/**
		* [EN]
		* Creates a new Runtime task from callable, selecting the
		* PreemptiveRuntime or NonpreemptiveRuntime alternative based on
		* which runtime reference type callable accepts, and returns a
		* handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable から新しい Runtime タスクを生成する。callable が
		* 受け取るランタイム参照の型に応じて PreemptiveRuntime または
		* NonpreemptiveRuntime の選択肢を選び、そのハンドルを返す。
		*/
		template<RuntimeTaskLike C>
		JobTask emplace(C&& callable)
		{
			/// [EN] The runtime type the callable accepts decides whether the node may suspend to wait for what it spawns.
			/// [JP] 処理が受け取るランタイムの型で、生成したものを待つためにノードが中断できるかが決まる。
			if constexpr (std::is_invocable_v<C, JobPreemptiveRuntime&>)
			{
				return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::PreemptiveRuntime>{}, std::forward<C>(callable)));
			}
			else if constexpr (std::is_invocable_v<C, JobNonpreemptiveRuntime&>)
			{
				return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::NonpreemptiveRuntime>{}, std::forward<C>(callable)));
			}
		}

		/**
		* [EN]
		* Creates a new Subflow task from callable (which receives a
		* JobSubflow& builder to construct the nested graph at runtime)
		* and returns a handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable（実行時にネストされたグラフを構築するための JobSubflow&
		* ビルダーを受け取る）から新しい Subflow タスクを生成し、その
		* ハンドルを返す。
		*/
		template<SubflowTaskLike C>
		JobTask emplace(C&& callable)
		{
			/// [EN] A new node starts with no state, no topology and no parent; those are filled in when a run is prepared.
			/// [JP] 新しいノードは状態・トポロジー・親を持たずに始まる。それらは実行の準備のときに埋まる。
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::Subflow>{}, std::forward<C>(callable)));
		}

		/**
		* [EN]
		* Creates a new SingleCondition task from callable (returns the
		* index of the single successor to follow) and returns a handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable（辿るべき単一の後続インデックスを返す）から新しい
		* SingleCondition タスクを生成し、そのハンドルを返す。
		*/
		template<SingleConditionTaskLike C>
		JobTask emplace(C&& callable)
		{
			/// [EN] A new node starts with no state, no topology and no parent; those are filled in when a run is prepared.
			/// [JP] 新しいノードは状態・トポロジー・親を持たずに始まる。それらは実行の準備のときに埋まる。
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::SingleCondition>{}, std::forward<C>(callable)));
		}

		/**
		* [EN]
		* Creates a new MultiCondition task from callable (returns the
		* indices of every successor to follow) and returns a handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable（辿るべきすべての後続インデックスを返す）から新しい
		* MultiCondition タスクを生成し、そのハンドルを返す。
		*/
		template<MultiConditionTaskLike C>
		JobTask emplace(C&& callable)
		{
			/// [EN] A new node starts with no state, no topology and no parent; those are filled in when a run is prepared.
			/// [JP] 新しいノードは状態・トポロジー・親を持たずに始まる。それらは実行の準備のときに埋まる。
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::MultiCondition>{}, std::forward<C>(callable)));
		}

		/**
		* [EN]
		* Creates one task per entry in callables (each dispatched via
		* the single-callable emplace overloads) and returns them as a tuple.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callables の各要素に対して 1 つずつタスクを生成し（それぞれ単一の
		* callable 版 emplace オーバーロード経由でディスパッチされる）、
		* それらをタプルとして返す。
		*/
		template<typename... C>
			requires(sizeof...(C) > 1)
		auto emplace(C&&... callables)
		{
			/// [EN] Each callable goes through the single-callable overloads, so every kind of task can be mixed in one call.
			/// [JP] 各処理は単一版のオーバーロードを通るので、1回の呼び出しに種類の違うタスクを混ぜられる。
			return std::make_tuple(emplace(std::forward<C>(callables))...);
		}

		/**
		* [EN]
		* Removes task from the graph, first detaching it from every
		* node it is connected to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* task をグラフから削除する。先に、つながっている全ノードから
		* task を切り離す。
		*/
		void Erase(JobTask task);

		/**
		* [EN]
		* Creates a new module task that runs callable's graph (owned
		* externally, so callable must outlive every run) and returns a
		* handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable のグラフ（外部が所有するので、callable は全ての実行より
		* 長く生きている必要がある）を実行する新しいモジュールタスクを
		* 生成し、そのハンドルを返す。
		*/
		template<GraphLike C>
		JobTask Composed(C& callable)
		{
			/// [EN] RetrieveGraph accepts both a JobGraph and anything exposing graph(); the node only keeps a reference to it.
			/// [JP] RetrieveGraph は JobGraph そのものも graph() を持つものも受け付ける。ノードはそれへの参照だけを持つ。
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::OwnedModule>{}, RetrieveGraph(callable)));
		}

		/**
		* [EN]
		* Creates a new module task that takes ownership of graph (moved
		* in) and returns a handle to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* graph の所有権を（ムーブで）引き受ける新しいモジュールタスクを
		* 生成し、そのハンドルを返す。
		*/
		JobTask Adopt(JobGraph&& graph);

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
		JobTask Placeholder();

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
		void Linearize(DynamicArray<JobTask>& keys);

		/**
		* [EN]
		* Chains every task in keys into a straight-line sequence, from
		* an initializer list.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* keys の各タスクを直線的な順序に連結する（DynamicArray
		* オーバーロードを参照）。初期化子リストから受け取る。
		*/
		void Linearize(std::initializer_list<JobTask>& keys);

	protected:
		/// [EN] The graph this builder creates/modifies tasks in; protected so JobSubflow can read it.
		/// [JP] このビルダーがタスクを生成・変更する対象のグラフ。JobSubflow から読めるよう protected にしている。
		JobGraph& graph_;

	private:
		/**
		* [EN]
		* Shared implementation for Linearize: chains any range-like
		* container of JobTask into a straight-line sequence.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Linearize の共通実装: JobTask の任意の範囲的コンテナを直線的な
		* 順序に連結する。
		*/
		template<typename L>
		void Linearize(L& keys)
		{
			auto iterator = keys.begin();
			auto end = keys.end();

			if (iterator == end)
			{
				return;
			}

			/// [EN] next runs one step ahead of iterator, so each pair of neighbours gets exactly one edge.
			/// [JP] next は iterator の1つ先を進むので、隣り合う組ごとにちょうど1本のエッジが張られる。
			auto next = iterator;

			for (++next;next != end;++next, ++iterator)
			{
				iterator->node_->Precede(next->node_);
			}
		}
	};

	/**
	* [EN]
	* FlowBuilder for a Subflow task's nested subgraph, created by the
	* executor when the task runs. It builds the subgraph and controls
	* whether the subgraph is joined (waited for) inside the task, and
	* whether it is kept after the task finishes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Subflow タスクのネストされたサブグラフ用の FlowBuilder。タスクの
	* 実行時にエグゼキュータが作る。サブグラフを構築し、タスクの中で
	* サブグラフに合流（完了を待機）するか、タスクが終わった後も
	* サブグラフを残すかを制御する。
	*/
	class JobSubflow :public FlowBuilder
	{
	private:
		/// [EN] Only the executor creates a subflow, through the private constructor.
		/// [JP] サブフローを作るのはエグゼキュータだけで、private なコンストラクタを通す。
		friend class JobExecutor;
		friend class FlowBuilder;

	public:
		/**
		* [EN]
		* Runs this subflow's subgraph to completion on the current
		* worker, helping with other work while waiting, and marks the
		* subflow as joined. Throws if it was already joined.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このサブフローのサブグラフを、今のワーカー上で完了まで実行する。
		* 待つ間は他の処理を手伝い、終わったら合流済みにする。既に合流済み
		* なら例外を投げる。
		*/
		void Join();

		/**
		* [EN]
		* Returns whether this subflow's subgraph is currently joinable
		* (has not already been joined).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このサブフローのサブグラフが現在 join 可能か（まだ join されて
		* いないか）を返す。
		*/
		Bool Joinable()const noexcept;

		/**
		* [EN]
		* Returns a reference to the JobExecutor running this subflow.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このサブフローを実行している JobExecutor への参照を返す。
		*/
		JobExecutor& Executor()noexcept;

		/**
		* [EN]
		* Returns a reference to this subflow's underlying graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このサブフローの内部グラフへの参照を返す。
		*/
		JobGraph& Graph();

		/**
		* [EN]
		* Sets whether this subflow's subgraph should be retained (kept
		* intact for reuse on the next execution) instead of being
		* cleared after it finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このサブフローのサブグラフを、完了後にクリアするのではなく
		* 保持する（次回実行時の再利用のためそのまま残す）かどうかを
		* 設定する。
		*/
		void Retain(Bool flag)noexcept;

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
		Bool Retain()const;

	private:
		/**
		* [EN]
		* Constructs a subflow builder for node's subgraph, running under
		* executor/worker, and resets the subgraph and the joined/retain
		* flags for this run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* executor/worker のもとで実行される、node のサブグラフに対する
		* サブフロービルダーを構築し、この実行に向けてサブグラフと
		* 合流済み/保持のフラグを初期化する。
		*/
		JobSubflow(JobExecutor& executor, JobWorker& worker, JobNode* node, JobGraph& graph);

		/**
		* [EN]
		* Default construction is disabled: a JobSubflow must always be
		* bound to an executor, worker, and node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルト構築は禁止されている: JobSubflow は常に executor、
		* worker、node に紐づいている必要がある。
		*/
		JobSubflow() = delete;

		/**
		* [EN]
		* Copy construction is disabled: a subflow is tied to one node's
		* run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止されている。サブフローは1つのノードの1回の実行に
		* 結びついている。
		*/
		JobSubflow(const JobSubflow&) = delete;

		/**
		* [EN]
		* Move construction is disabled, for the same reason as copying.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ムーブ構築も、コピーと同じ理由で禁止されている。
		*/
		JobSubflow(JobSubflow&&) = delete;

		/// [EN] The executor running this subflow.
		/// [JP] このサブフローを実行しているエグゼキュータ。
		JobExecutor& executor_;

		/// [EN] The worker running the Subflow task; Join coruns the subgraph on it.
		/// [JP] Subflow タスクを実行しているワーカー。Join はこの上でサブグラフを Corun する。
		JobWorker& worker_;

		/// [EN] The Subflow-type node that owns this subflow's subgraph and carries its joined/retain flags.
		/// [JP] このサブフローのサブグラフを所有し、合流済み/保持のフラグを持つ、Subflow 種別のノード。
		JobNode* node_;
	};
}
