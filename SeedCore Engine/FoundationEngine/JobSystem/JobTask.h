#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/JobSystem/JobDeclaretions.h>
#include <FoundationEngine/JobSystem/JobConcept.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	/// [EN] Identifies the kind of work a JobTask/JobNode performs, mirroring JobNode::NodeHandle's variant alternatives.
	/// [JP] JobTask/JobNode が行う処理の種類を表す。JobNode::NodeHandle のバリアントの選択肢に対応する。
	enum class JobTaskType :Int
	{
		/// [EN] No work has been assigned yet.
		/// [JP] まだ処理が割り当てられていない。
		PLACEHOLDER = 0,

		/// [EN] A plain static (parameterless) callable.
		/// [JP] 単純な静的（引数なし）の呼び出し可能オブジェクト。
		STATIC,

		/// [EN] Work that runs under a preemptive or non-preemptive runtime.
		/// [JP] プリエンプティブ、または非プリエンプティブなランタイム上で実行される処理。
		RUNTIME,

		/// [EN] A dynamically-built subflow.
		/// [JP] 動的に構築されるサブフロー。
		SUBFLOW,

		/// [EN] A conditional branch (single or multi successor selection).
		/// [JP] 条件分岐（単一または複数の後続選択）。
		CONDITION,

		/// [EN] A module wrapping an owned or adopted JobGraph.
		/// [JP] 所有または養子化された JobGraph を包むモジュール。
		MODULE,

		/// [EN] Sentinel returned for a handle alternative with no matching kind.
		/// [JP] 対応する種類が無いハンドルの選択肢に対して返す番兵値。
		UNDEFINED
	};

	/// [EN] Enumerates every concrete (non-sentinel) JobTaskType value, for iteration/lookup.
	/// [JP] （番兵値を除く）すべての具体的な JobTaskType 値を列挙する。走査・検索に用いる。
	inline constexpr auto JOB_TASK_TYPES = std::to_array<JobTaskType>
		({
			JobTaskType::PLACEHOLDER,
			JobTaskType::STATIC,
			JobTaskType::RUNTIME,
			JobTaskType::SUBFLOW,
			JobTaskType::CONDITION,
			JobTaskType::MODULE,
		});

	/**
	* [EN]
	* Returns the display name of type as a null-terminated string
	* (defined in JobTask.cpp).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* type の表示名をヌル終端文字列として返す（定義は JobTask.cpp）。
	*/
	SEEDCORE_API const Char* ToString(JobTaskType type);

	/// [EN] Satisfied when C is invocable with no arguments and returns void (a Static task callable).
	/// [JP] C が引数なしで呼び出し可能で void を返す場合に満たされる（Static タスクの呼び出し可能条件）。
	template<typename C>
	concept StaticTaskLike = std::invocable<C> && std::same_as<std::invoke_result_t<C>, void>;

	/// [EN] Compile-time boolean value mirroring StaticTaskLike.
	/// [JP] StaticTaskLike をミラーするコンパイル時のブール値。
	template<typename C>
	constexpr Bool IsStaticTaskValue = StaticTaskLike<C>;

	/// [EN] Satisfied when C is invocable with a Subflow& and returns void (a Subflow task callable).
	/// [JP] C が Subflow& を受け取って呼び出し可能で void を返す場合に満たされる（Subflow タスクの呼び出し可能条件）。
	template<typename C>
	concept SubflowTaskLike = std::invocable<C, Subflow&>&& std::same_as<std::invoke_result_t<C, Subflow&>, void>;

	/// [EN] Compile-time boolean value mirroring SubflowTaskLike.
	/// [JP] SubflowTaskLike をミラーするコンパイル時のブール値。
	template<typename C>
	constexpr Bool IsSubflowTaskValue = SubflowTaskLike<C>;

	/// [EN] Satisfied when C is invocable with a JobPreemptiveRuntime& or JobNonpreemptiveRuntime& and returns void (a Runtime task callable).
	/// [JP] C が JobPreemptiveRuntime& または JobNonpreemptiveRuntime& を受け取って呼び出し可能で void を返す場合に満たされる（Runtime タスクの呼び出し可能条件）。
	template<typename C>
	concept RuntimeTaskLike =
		(std::invocable<C, JobPreemptiveRuntime&> && std::same_as<std::invoke_result_t<C, JobPreemptiveRuntime&>, void>) ||
		(std::invocable<C, JobNonpreemptiveRuntime&> && std::same_as<std::invoke_result_t<C, JobNonpreemptiveRuntime&>, void>);

	/// [EN] Compile-time boolean value mirroring RuntimeTaskLike.
	/// [JP] RuntimeTaskLike をミラーするコンパイル時のブール値。
	template<typename C>
	constexpr Bool IsRuntimeTaskValue = RuntimeTaskLike<C>;

	/// [EN] Satisfied when C is invocable with no arguments and returns something convertible to Int (a SingleCondition task callable).
	/// [JP] C が引数なしで呼び出し可能で、戻り値が Int に変換可能な場合に満たされる（SingleCondition タスクの呼び出し可能条件）。
	template<typename C>
	concept SingleConditionTaskLike = std::invocable<C> && std::convertible_to<std::invoke_result_t<C>, Int>;

	/// [EN] Compile-time boolean value mirroring SingleConditionTaskLike.
	/// [JP] SingleConditionTaskLike をミラーするコンパイル時のブール値。
	template<typename C>
	constexpr Bool IsSingleConditionTaskValue = SingleConditionTaskLike<C>;

	/// [EN] Satisfied when C is invocable with no arguments and returns HybridArray<Int> (a MultiCondition task callable).
	/// [JP] C が引数なしで呼び出し可能で HybridArray<Int> を返す場合に満たされる（MultiCondition タスクの呼び出し可能条件）。
	template<typename C>
	concept MultiConditionTaskLike = std::invocable<C> && std::same_as<std::invoke_result_t<C>, HybridArray<Int>>;

	/// [EN] Compile-time boolean value mirroring MultiConditionTaskLike.
	/// [JP] MultiConditionTaskLike をミラーするコンパイル時のブール値。
	template<typename C>
	constexpr Bool IsMultiConditionTaskValue = MultiConditionTaskLike<C>;

	/**
	* [EN]
	* Lightweight, copyable handle to a single JobNode owned by a
	* JobGraph. Provides the public graph-building API: assigning work,
	* wiring dependencies, attaching semaphores, and querying execution
	* state. Does not own the underlying node; the node's lifetime is
	* managed by its owning JobGraph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JobGraph が所有する単一の JobNode への、軽量でコピー可能な
	* ハンドル。処理の割り当て、依存関係の配線、セマフォの付与、実行状態の
	* 問い合わせといった、公開されたグラフ構築 API を提供する。内部の
	* ノード自体は所有しない。ノードのライフタイムは、それを所有する
	* JobGraph によって管理される。
	*/
	class SEEDCORE_API JobTask
	{
	private:
		/// [EN] Builders and runtimes create handles from nodes and reach node_ directly.
		/// [JP] ビルダーとランタイムは、ノードからハンドルを作り、node_ に直接触れる。
		friend class FlowBuilder;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;
		friend class JobTaskflow;
		friend class JobTaskView;
		friend class JobExecutor;

	public:
		/**
		* [EN]
		* Default constructor: creates a handle pointing at no node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: どのノードも指さないハンドルを生成する。
		*/
		JobTask() = default;

		/**
		* [EN]
		* Copy-constructs, pointing at the same underlying node as rhs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* rhs と同じ内部ノードを指すようにコピー構築する。
		*/
		JobTask(const JobTask& rhs);

		/**
		* [EN]
		* Copy-assigns, pointing at the same underlying node as rhs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* rhs と同じ内部ノードを指すようにコピー代入する。
		*/
		JobTask& operator=(const JobTask& rhs);

		/**
		* [EN]
		* Resets this handle to point at no node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このハンドルをどのノードも指さない状態にリセットする。
		*/
		JobTask& operator=(std::nullptr_t null);

		/**
		* [EN]
		* Returns whether this and rhs refer to the same underlying node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* this と rhs が同じ内部ノードを指しているかどうかを返す。
		*/
		Bool operator==(const JobTask& rhs)const;

		/**
		* [EN]
		* Negation of operator==.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* operator== の否定。
		*/
		Bool operator!=(const JobTask& rhs)const;

		/**
		* [EN]
		* Returns the underlying node's display name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードの表示名を返す。
		*/
		const String& Name()const;

		/**
		* [EN]
		* Returns the number of successor tasks connected to this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクに接続されている後続タスクの数を返す。
		*/
		Size NumberSuccessors()const;

		/**
		* [EN]
		* Returns the number of predecessor tasks connected to this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクに接続されている先行タスクの数を返す。
		*/
		Size NumberPredecessors()const;

		/**
		* [EN]
		* Returns the number of strong (unconditional) dependencies this task has.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクが持つ強い（無条件の）依存関係の数を返す。
		*/
		Size NumberStrongDependencies()const;

		/**
		* [EN]
		* Returns the number of weak (conditional-branch-only) dependencies this task has.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクが持つ弱い（条件分岐経由のみの）依存関係の数を返す。
		*/
		Size NumberWeakDependencies()const;

		/**
		* [EN]
		* Sets the underlying node's display name and returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードの表示名を設定し、メソッドチェーン用に *this を返す。
		*/
		JobTask& Name(const String& name);

		/**
		* [EN]
		* Assigns callable as this task's work, selecting the appropriate
		* NodeHandle alternative (Static/Runtime/Subflow/SingleCondition/
		* MultiCondition) based on which concept callable satisfies.
		* Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* callable が満たすコンセプトに応じて適切な NodeHandle の選択肢
		* （Static/Runtime/Subflow/SingleCondition/MultiCondition）を選び、
		* このタスクの処理として割り当てる。メソッドチェーン用に *this を返す。
		*/
		template<typename C>
		JobTask& Work(C&& callable)
		{
			/// [EN] Replacing the handle alternative destroys the previous work, whatever kind it was.
			/// [JP] ハンドルの選択肢を置き換えると、前の処理は種類に関係なく破棄される。
			if constexpr (IsStaticTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::Static>(std::forward<C>(callable));
			}
			else if constexpr (IsRuntimeTaskValue<C>)
			{
				/// [EN] The runtime type the callable accepts decides between the preemptive and non-preemptive alternatives, as in FlowBuilder::emplace.
				/// [JP] 処理が受け取るランタイムの型で、FlowBuilder::emplace と同じく、プリエンプティブか非プリエンプティブかを選ぶ。
				if constexpr (std::is_invocable_v<C, JobPreemptiveRuntime&>)
				{
					node_->handle_.emplace<JobNode::PreemptiveRuntime>(std::forward<C>(callable));
				}
				else
				{
					node_->handle_.emplace<JobNode::NonpreemptiveRuntime>(std::forward<C>(callable));
				}
			}
			else if constexpr (IsSubflowTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::Subflow>(std::forward<C>(callable));
			}
			else if constexpr (IsSingleConditionTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::SingleCondition>(std::forward<C>(callable));
			}
			else if constexpr (IsMultiConditionTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::MultiCondition>(std::forward<C>(callable));
			}
			/// [EN] A callable matching none of the kinds leaves the current work unchanged.
			/// [JP] どの種類にも当てはまらない処理では、今の処理をそのまま残す。
			else
			{

			}
			return *this;
		}

		/**
		* [EN]
		* Turns this task into a module that runs target's graph (owned
		* externally, so target must outlive every run). Returns *this for
		* chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* target のグラフ（外部が所有するので、target は全ての実行より長く
		* 生きている必要がある）を実行するモジュールへこのタスクを変換する。
		* メソッドチェーン用に *this を返す。
		*/
		template<GraphLike T>
		JobTask& Composed(T& target)
		{
			node_->handle_.emplace<JobNode::OwnedModule>(RetrieveGraph(target));
			return *this;
		}

		/**
		* [EN]
		* Turns this task into a module that takes ownership of graph
		* (moved in). Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* graph の所有権を（ムーブで）引き受けるモジュールへこのタスクを
		* 変換する。メソッドチェーン用に *this を返す。
		*/
		JobTask& Adopt(JobGraph&& graph);

		/**
		* [EN]
		* Establishes this task as a predecessor of every task in tasks.
		* Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* tasks の各タスクに対して、このタスクを先行タスクとして設定する。
		* メソッドチェーン用に *this を返す。
		*/
		template<typename... Ts>
		JobTask& Precede(Ts&&... tasks)
		{
			/// [EN] One edge per task, added in argument order.
			/// [JP] タスクごとに1本ずつ、引数の順にエッジを足す。
			(node_->Precede(tasks.node_), ...);
			return *this;
		}

		/**
		* [EN]
		* Establishes this task as a successor of every task in tasks.
		* Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* tasks の各タスクに対して、このタスクを後続タスクとして設定する。
		* メソッドチェーン用に *this を返す。
		*/
		template<typename... Ts>
		JobTask& Succeed(Ts&&... tasks)
		{
			/// [EN] The same edges as Precede, drawn from the other end.
			/// [JP] Precede と同じエッジを、反対側から張る。
			(tasks.node_->Precede(node_), ...);
			return *this;
		}

		/**
		* [EN]
		* Removes the precedence edges between this task and every task
		* in tasks (as predecessors of this task). Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクと tasks の各タスク（このタスクの先行タスクとして）
		* との間の先行関係エッジを削除する。メソッドチェーン用に *this を返す。
		*/
		template<typename... Ts>
		JobTask& RemovePredecessors(Ts&&... tasks)
		{
			/// [EN] An edge is recorded on both nodes, so it is removed from each side.
			/// [JP] エッジは両方のノードに記録されているので、それぞれの側から取り除く。
			(tasks.node_->RemoveSuccessors(node_), ...);
			(node_->RemovePredecessors(tasks.node_), ...);
			return *this;
		}

		/**
		* [EN]
		* Removes the precedence edges between this task and every task
		* in tasks (as successors of this task). Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクと tasks の各タスク（このタスクの後続タスクとして）
		* との間の先行関係エッジを削除する。メソッドチェーン用に *this を返す。
		*/
		template<typename... Ts>
		JobTask& RemoveSuccessors(Ts&&... tasks)
		{
			/// [EN] An edge is recorded on both nodes, so it is removed from each side.
			/// [JP] エッジは両方のノードに記録されているので、それぞれの側から取り除く。
			(node_->RemoveSuccessors(tasks.node_), ...);
			(tasks.node_->RemovePredecessors(node_), ...);
			return *this;
		}

		/**
		* [EN]
		* Registers semaphore to be released once this task finishes
		* executing. Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクの実行完了時に解放されるセマフォとして semaphore を
		* 登録する。メソッドチェーン用に *this を返す。
		*/
		JobTask& Release(Semaphore& semaphore);

		/**
		* [EN]
		* Registers every semaphore in [first, last) to be released once
		* this task finishes executing. Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* [first, last) の各セマフォを、このタスクの実行完了時に解放される
		* ものとして登録する。メソッドチェーン用に *this を返す。
		*/
		template<typename I>
		JobTask& Release(I first, I last)
		{
			/// [EN] The semaphore block is created only once a task actually uses one, keeping ordinary nodes small.
			/// [JP] セマフォ用のまとまりは、実際に使うタスクで初めて作る。普通のノードは小さいままで済む。
			if (!node_->semaphores_)
			{
				node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
			}
			node_->semaphores_->release_.reserve(node_->semaphores_->release_.size() + std::distance(first, last));
			for (auto s = first;s != last;++s)
			{
				node_->semaphores_->release_.push_back(&(*s));
			}
			return *this;
		}

		/**
		* [EN]
		* Registers semaphore to be acquired before this task may
		* execute. Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクが実行可能になる前に獲得すべきセマフォとして
		* semaphore を登録する。メソッドチェーン用に *this を返す。
		*/
		JobTask& Acquire(Semaphore& semaphore);

		/**
		* [EN]
		* Registers every semaphore in [first, last) to be acquired
		* before this task may execute. Returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* [first, last) の各セマフォを、このタスクが実行可能になる前に
		* 獲得すべきものとして登録する。メソッドチェーン用に *this を返す。
		*/
		template<typename I>
		JobTask& Acquire(I first, I last)
		{
			if (!node_->semaphores_)
			{
				node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
			}

			/// [EN] Only pointers are stored, so every semaphore must outlive the task's runs.
			/// [JP] 持つのはポインタだけなので、各セマフォはタスクの実行より長く生きている必要がある。
			node_->semaphores_->acquire_.reserve(node_->semaphores_->acquire_.size() + std::distance(first, last));
			for (auto s = first;s != last;++s)
			{
				node_->semaphores_->acquire_.push_back(&(*s));
			}
			return *this;
		}

		/**
		* [EN]
		* Sets the underlying node's user-data pointer and returns *this for chaining.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードのユーザーデータポインタを設定し、メソッドチェーン用に
		* *this を返す。
		*/
		JobTask& Data(void* data);

		/**
		* [EN]
		* Resets this handle to point at no node; the node itself is left
		* untouched.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このハンドルをどのノードも指さない状態にリセットする。ノード自体
		* には触れない。
		*/
		void Reset();

		/**
		* [EN]
		* Clears only the assigned work (NodeHandle), leaving
		* dependencies and semaphores intact.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 割り当てられた処理（NodeHandle）のみをクリアし、依存関係と
		* セマフォはそのまま維持する。
		*/
		void ResetWork();

		/**
		* [EN]
		* Returns whether this handle points at no node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このハンドルがどのノードも指していないかどうかを返す。
		*/
		Bool Empty()const;

		/**
		* [EN]
		* Returns whether this task has been assigned any work (its
		* NodeHandle is not the Placeholder alternative).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクに何らかの処理が割り当てられているか（NodeHandle が
		* Placeholder 以外であるか）を返す。
		*/
		Bool HasWork()const;

		/**
		* [EN]
		* Invokes visitor(JobTask) for every successor of this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクの各後続タスクに対して visitor(JobTask) を呼び出す。
		*/
		template<typename V>
		void EachSuccessor(V&& visitor)const
		{
			/// [EN] Successors occupy the front numberSuccessors_ entries of edges_.
			/// [JP] 後続は edges_ の先頭 numberSuccessors_ 個に並んでいる。
			for (Size index = 0;index < node_->numberSuccessors_;++index)
			{
				visitor(JobTask(node_->edges_[index]));
			}
		}

		/**
		* [EN]
		* Invokes visitor(JobTask) for every predecessor of this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクの各先行タスクに対して visitor(JobTask) を呼び出す。
		*/
		template<typename V>
		void EachPredecessor(V&& visitor)const
		{
			/// [EN] Predecessors are stored after the successors in edges_.
			/// [JP] edges_ では、先行ノードは後続の後ろに並んでいる。
			for (Size index = node_->numberSuccessors_;index < node_->edges_.size();++index)
			{
				visitor(JobTask(node_->edges_[index]));
			}
		}

		/**
		* [EN]
		* If this task is a Subflow, invokes visitor(JobTask) for every
		* node currently in its nested subgraph; otherwise does nothing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクが Subflow であれば、そのネストされたサブグラフ内の
		* 各ノードに対して visitor(JobTask) を呼び出す。そうでなければ
		* 何もしない。
		*/
		template<typename V>
		void EachSubflowTask(V&& visitor)const
		{
			/// [EN] The subgraph holds whatever the last run built, unless it was cleared afterwards.
			/// [JP] サブグラフには、後で消されていなければ、直前の実行で作られたものが入っている。
			if (auto ptr = std::get_if<JobNode::Subflow>(&node_->handle_);ptr)
			{
				for (auto iterator = ptr->subgraph_.begin();iterator != ptr->subgraph_.end();++iterator)
				{
					visitor(JobTask(*iterator));
				}
			}
		}

		/**
		* [EN]
		* Returns a hash value identifying the underlying node (derived
		* from its address).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードを識別するハッシュ値を返す（アドレスから求める）。
		*/
		Size HashValue()const;

		/**
		* [EN]
		* Returns the kind of work this task currently performs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクが現在行っている処理の種類を返す。
		*/
		JobTaskType Type()const;

		/**
		* [EN]
		* Returns the underlying node's user-data pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードのユーザーデータポインタを返す。
		*/
		void* Data()const;

		/**
		* [EN]
		* Returns the exception stored on this task's node, if any;
		* nullptr for an empty handle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクのノードに格納された例外があれば、それを返す。空の
		* ハンドルでは nullptr。
		*/
		std::exception_ptr ExceptionPtr()const;

		/**
		* [EN]
		* Returns whether an exception has been propagated to this task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このタスクに例外が伝播しているかどうかを返す。
		*/
		Bool HasExceptionPtr()const;

	private:
		/**
		* [EN]
		* Constructs a handle wrapping node directly.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node を直接包むハンドルを構築する。
		*/
		JobTask(JobNode* node);

		/// [EN] The node this handle refers to; nullptr if empty.
		/// [JP] このハンドルが参照するノード。空であれば nullptr。
		JobNode* node_ = nullptr;
	};

	/**
	* [EN]
	* Read-only, non-owning view onto a JobNode, exposing the same
	* query surface as JobTask (name, dependency counts, iteration,
	* type/hash) but without the graph-building/mutation API. Used where
	* callers should be able to inspect a node without being able to modify it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JobNode に対する読み取り専用・非所有のビュー。JobTask と同じ
	* 問い合わせ用インターフェース（名前・依存関係数・走査・種別/ハッシュ）
	* を公開するが、グラフ構築・変更用の API は持たない。呼び出し側が
	* ノードを変更できないようにしつつ参照だけはできるようにしたい場合に
	* 使う。
	*/
	class JobTaskView
	{
	private:
		/// [EN] Only the executor creates views, to hand nodes out without allowing changes.
		/// [JP] ビューを作るのはエグゼキュータだけで、変更を許さずにノードを渡すために使う。
		friend class JobExecutor;

	public:
		/**
		* [EN]
		* Returns the underlying node's display name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードの表示名を返す。
		*/
		const String& Name()const;

		/**
		* [EN]
		* Returns the number of successor nodes connected to this node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードに接続されている後続ノードの数を返す。
		*/
		Size NumberSuccessors()const;

		/**
		* [EN]
		* Returns the number of predecessor nodes connected to this node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードに接続されている先行ノードの数を返す。
		*/
		Size NumberPredecessors()const;

		/**
		* [EN]
		* Returns the number of strong (unconditional) dependencies this node has.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが持つ強い（無条件の）依存関係の数を返す。
		*/
		Size NumberStrongDependencies()const;

		/**
		* [EN]
		* Returns the number of weak (conditional-branch-only) dependencies this node has.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが持つ弱い（条件分岐経由のみの）依存関係の数を返す。
		*/
		Size NumberWeakDependencies()const;

		/**
		* [EN]
		* Invokes visitor(JobTaskView) for every successor of this node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードの各後続ノードに対して visitor(JobTaskView) を呼び出す。
		*/
		template<typename V>
		void EachSuccessor(V&& visitor)const
		{
			/// [EN] Successors occupy the front numberSuccessors_ entries of edges_.
			/// [JP] 後続は edges_ の先頭 numberSuccessors_ 個に並んでいる。
			for (Size index = 0;index < node_.numberSuccessors_;++index)
			{
				visitor(JobTaskView(*node_.edges_[index]));
			}
		}

		/**
		* [EN]
		* Invokes visitor(JobTaskView) for every predecessor of this node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードの各先行ノードに対して visitor(JobTaskView) を呼び出す。
		*/
		template<typename V>
		void EachPredecessor(V&& visitor)const
		{
			/// [EN] Predecessors are stored after the successors in edges_.
			/// [JP] edges_ では、先行ノードは後続の後ろに並んでいる。
			for (Size index = node_.numberSuccessors_;index < node_.edges_.size();++index)
			{
				visitor(JobTaskView(*node_.edges_[index]));
			}
		}

		/**
		* [EN]
		* Returns the kind of work this node currently performs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが現在行っている処理の種類を返す。
		*/
		JobTaskType Type()const;

		/**
		* [EN]
		* Returns a hash value identifying the underlying node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部ノードを識別するハッシュ値を返す。
		*/
		Size HashValue()const;

	private:
		/**
		* [EN]
		* Constructs a view referring to node directly.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node を直接参照するビューを構築する。
		*/
		JobTaskView(const JobNode& node);

		/**
		* [EN]
		* Copy-constructs, referring to the same underlying node as the source view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー元のビューと同じ内部ノードを参照するようにコピー構築する。
		*/
		JobTaskView(const JobTaskView&) = default;

		/// [EN] The node this view refers to; a reference, so a view can never be empty.
		/// [JP] このビューが参照するノード。参照なので、ビューが空になることは無い。
		const JobNode& node_;
	};
}

namespace std
{
	/**
	* [EN]
	* std::hash specialization for JobTask, delegating to
	* JobTask::HashValue so JobTask can be used as a key in unordered
	* containers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JobTask 向けの std::hash 特殊化。JobTask::HashValue へ委譲し、
	* JobTask を unordered コンテナのキーとして使用できるようにする。
	*/
	template<>
	struct hash<SeedCore::JobTask>
	{
		/**
		* [EN]
		* Returns task's hash value.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* task のハッシュ値を返す。
		*/
		auto operator()(const SeedCore::JobTask& task)const noexcept
		{
			return task.HashValue();
		}
	};

	/**
	* [EN]
	* std::hash specialization for JobTaskView, delegating to
	* JobTaskView::HashValue so JobTaskView can be used as a key in
	* unordered containers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JobTaskView 向けの std::hash 特殊化。JobTaskView::HashValue へ
	* 委譲し、JobTaskView を unordered コンテナのキーとして使用できる
	* ようにする。
	*/
	template<>
	struct hash<SeedCore::JobTaskView>
	{
		/**
		* [EN]
		* Returns taskView's hash value.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* taskView のハッシュ値を返す。
		*/
		auto operator()(const SeedCore::JobTaskView& taskView)const noexcept
		{
			return taskView.HashValue();
		}
	};
}
