#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/JobSystem/JobCommon.h>
#include <FoundationEngine/JobSystem/JobNodeBase.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobTopology.h>
#include <FoundationEngine/JobSystem/Semaphore.h>
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	/**
	* [EN]
	* Represents a single node in a job graph. Holds the node's identity
	* (name/data), its position within the topology, its edges to other
	* nodes, and a NodeHandle variant describing what kind of work
	* (static function, runtime, subflow, condition, module, etc.) the
	* node actually performs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョブグラフ内の単一のノードを表すクラス。ノードの識別情報
	* （名前・データ）、トポロジー内での位置、他ノードへのエッジ、
	* および実際にどのような処理（静的関数、ランタイム、サブフロー、
	* 条件分岐、モジュールなど）を行うかを表す NodeHandle バリアントを
	* 保持する。
	*/
	class JobNode :public JobNodeBase
	{
	private:
		/// [EN] Everything that builds, inspects or runs graphs works on the node's private state directly.
		/// [JP] グラフを組み立てる・調べる・実行するクラスは、ノードの private な状態を直接扱う。
		friend class JobGraph;
		friend class JobTask;
		friend class JobTaskView;
		friend class JobTaskflow;
		friend class JobExecutor;
		friend class FlowBuilder;
		friend class JobSubflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;

	private:
		/// [EN] Marker type representing a node that has not yet been assigned any work (the default/empty alternative of NodeHandle).
		/// [JP] まだ処理が割り当てられていないノードを表すマーカー型（NodeHandle のデフォルト・空の選択肢）。
		using Placeholder = std::monostate;

		/**
		* [EN]
		* Node handle alternative for a plain static (parameterless)
		* callable that is executed when the node runs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ノード実行時に呼び出される、単純な静的（引数なし）の呼び出し
		* 可能オブジェクトを表す NodeHandle の選択肢。
		*/
		struct Static
		{
			/**
			* [EN]
			* Constructs, storing c as the callable to run when this node executes.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* c を、このノード実行時に呼び出す処理として保持して構築する。
			*/
			template<typename C>
			Static(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			/// [EN] The work to execute when this node runs.
			/// [JP] このノードが実行される際に呼び出される処理。
			std::function<void()> work_;
		};

		/**
		* [EN]
		* Node handle alternative for work that receives a
		* JobPreemptiveRuntime&, through which it can spawn more tasks;
		* the node stays suspended until those tasks have finished.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* JobPreemptiveRuntime& を受け取る処理を表す NodeHandle の選択肢。
		* それを通じてさらにタスクを生成でき、ノードはそれらのタスクが終わる
		* まで中断したままになる。
		*/
		struct PreemptiveRuntime
		{
			/**
			* [EN]
			* Constructs, storing c as the callable to run against the preemptive runtime.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* c を、プリエンプティブランタイムに対して実行する処理として
			* 保持して構築する。
			*/
			template<typename C>
			PreemptiveRuntime(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			/// [EN] The work to execute, given access to the preemptive runtime.
			/// [JP] プリエンプティブランタイムへのアクセスを受け取りながら実行される処理。
			std::function<void(JobPreemptiveRuntime&)> work_;
		};

		/**
		* [EN]
		* Node handle alternative for work that receives a
		* JobNonpreemptiveRuntime&; anything started through it must be
		* waited for (corun) before the work returns, so the node never
		* suspends.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* JobNonpreemptiveRuntime& を受け取る処理を表す NodeHandle の選択肢。
		* それを通じて始めたものは処理が戻る前に（Corun で）待つ必要があり、
		* ノードが中断することはない。
		*/
		struct NonpreemptiveRuntime
		{
			/**
			* [EN]
			* Constructs, storing c as the callable to run against the non-preemptive runtime.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* c を、非プリエンプティブランタイムに対して実行する処理として
			* 保持して構築する。
			*/
			template<typename C>
			NonpreemptiveRuntime(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			/// [EN] The work to execute, given access to the non-preemptive runtime.
			/// [JP] 非プリエンプティブランタイムへのアクセスを受け取りながら実行される処理。
			std::function<void(JobNonpreemptiveRuntime&)> work_;
		};

		/**
		* [EN]
		* Node handle alternative representing a dynamically-built
		* subflow: a nested JobGraph that is populated at runtime by
		* user-supplied work that receives a JobSubflow& builder.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 動的に構築されるサブフローを表す NodeHandle の選択肢。
		* JobSubflow& ビルダーを受け取るユーザー定義の処理によって
		* 実行時に構築される、ネストされた JobGraph を保持する。
		*/
		struct Subflow
		{
			/**
			* [EN]
			* Constructs, storing c as the callable that builds the subgraph at runtime.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* c を、実行時にサブグラフを構築する処理として保持して構築する。
			*/
			template<typename C>
			Subflow(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			/// [EN] User-supplied work that builds the subgraph at runtime via the given subflow builder.
			/// [JP] 渡されたサブフロービルダーを通じて、実行時にサブグラフを構築するユーザー定義の処理。
			std::function<void(JobSubflow&)> work_;

			/// [EN] The nested graph populated by work_ when the subflow runs.
			/// [JP] サブフローが実行される際に work_ によって構築されるネストされたグラフ。
			JobGraph subgraph_;
		};

		/**
		* [EN]
		* Node handle alternative for a conditional branch that
		* evaluates to a single successor index, selecting exactly one
		* outgoing edge to follow.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単一の後続インデックスへ評価される条件分岐を表す NodeHandle の
		* 選択肢であり、辿るべき出力エッジをちょうど 1 つ選択する。
		*/
		struct SingleCondition
		{
			/**
			* [EN]
			* Constructs, storing c as the callable that returns the successor index to follow.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* c を、辿るべき後続インデックスを返す処理として保持して構築する。
			*/
			template <typename C>
			SingleCondition(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			/// [EN] Work that returns the index of the single successor edge to follow.
			/// [JP] 辿るべき単一の後続エッジのインデックスを返す処理。
			std::function<Int()> work_;
		};

		/**
		* [EN]
		* Node handle alternative for a conditional branch that
		* evaluates to multiple successor indices, selecting zero or
		* more outgoing edges to follow.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 複数の後続インデックスへ評価される条件分岐を表す NodeHandle の
		* 選択肢であり、辿るべき出力エッジを 0 個以上選択する。
		*/
		struct MultiCondition
		{
			/**
			* [EN]
			* Constructs, storing c as the callable that returns the successor indices to follow.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* c を、辿るべき後続インデックス群を返す処理として保持して構築する。
			*/
			template <typename C>
			MultiCondition(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			/// [EN] Work that returns the indices of all successor edges to follow.
			/// [JP] 辿るべきすべての後続エッジのインデックスを返す処理。
			std::function<HybridArray<Int>()> work_;
		};

		/**
		* [EN]
		* Node handle alternative for a module whose underlying
		* JobGraph is owned externally; this node only holds a
		* reference to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部の JobGraph を外部が所有しているモジュールを表す
		* NodeHandle の選択肢であり、このノードはそれへの参照のみを保持する。
		*/
		struct OwnedModule
		{
			/**
			* [EN]
			* Constructs, storing a reference to the externally-owned graph.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 外部が所有するグラフへの参照を保持して構築する。
			*/
			OwnedModule(JobGraph& graph);

			/// [EN] Reference to the externally-owned graph this module wraps.
			/// [JP] このモジュールが包む、外部が所有しているグラフへの参照。
			JobGraph& graph_;
		};

		/**
		* [EN]
		* Node handle alternative for a module whose underlying
		* JobGraph is moved into and owned by this node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部の JobGraph がこのノードへムーブされ、所有されるモジュールを
		* 表す NodeHandle の選択肢。
		*/
		struct AdoptedModule
		{
			/**
			* [EN]
			* Constructs, moving graph into this module's ownership.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* graph をこのモジュールの所有権へムーブして構築する。
			*/
			AdoptedModule(JobGraph&& graph);

			/// [EN] The graph moved into and owned by this module.
			/// [JP] このモジュールへムーブされ、所有されているグラフ。
			JobGraph graph_;
		};

		/// [EN] Variant describing the kind of work this node performs; exactly one alternative is active at a time.
		/// [JP] このノードが行う処理の種類を表すバリアント。常にいずれか 1 つの選択肢のみが有効になる。
		using NodeHandle = std::variant
			<
			Placeholder,
			Static,
			PreemptiveRuntime,
			NonpreemptiveRuntime,
			Subflow,
			SingleCondition,
			MultiCondition,
			OwnedModule,
			AdoptedModule
			>;

		/**
		* [EN]
		* Groups the semaphores that must be acquired before this node
		* runs, and the semaphores that must be released after it runs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが実行される前に獲得しておく必要があるセマフォと、
		* 実行後に解放する必要があるセマフォをまとめて保持する。
		*/
		struct Semaphores
		{
			/// [EN] Semaphores that must all be taken before this node may run.
			/// [JP] このノードが実行できるようになる前に、全て取っておく必要があるセマフォ群。
			HybridArray<Semaphore*> acquire_;

			/// [EN] Semaphores given back once this node has finished running.
			/// [JP] このノードの実行が終わった後に返すセマフォ群。
			HybridArray<Semaphore*> release_;
		};

	public:
		/// [EN] Index of Placeholder within NodeHandle; these constants are compared against handle_.index() to tell the node kind.
		/// [JP] NodeHandle 内における Placeholder のインデックス。これらの定数を handle_.index() と比べてノードの種類を判定する。
		constexpr static auto PLACEHOLDER = GetIndexValue<Placeholder, NodeHandle>;
		
		/// [EN] Index of Static within NodeHandle.
		/// [JP] NodeHandle 内における Static のインデックス。
		constexpr static auto STATIC = GetIndexValue<Static, NodeHandle>;
		
		/// [EN] Index of PreemptiveRuntime within NodeHandle.
		/// [JP] NodeHandle 内における PreemptiveRuntime のインデックス。
		constexpr static auto PREEMPTIVE_RUNTIME = GetIndexValue<PreemptiveRuntime, NodeHandle>;
		
		/// [EN] Index of NonpreemptiveRuntime within NodeHandle.
		/// [JP] NodeHandle 内における NonpreemptiveRuntime のインデックス。
		constexpr static auto NONPREEMPTIVE_RUNTIME = GetIndexValue<NonpreemptiveRuntime, NodeHandle>;
		
		/// [EN] Index of Subflow within NodeHandle.
		/// [JP] NodeHandle 内における Subflow のインデックス。
		constexpr static auto SUBFLOW = GetIndexValue<Subflow, NodeHandle>;
		
		/// [EN] Index of SingleCondition within NodeHandle.
		/// [JP] NodeHandle 内における SingleCondition のインデックス。
		constexpr static auto SINGLE_CONDITION = GetIndexValue<SingleCondition, NodeHandle>;
		
		/// [EN] Index of MultiCondition within NodeHandle.
		/// [JP] NodeHandle 内における MultiCondition のインデックス。
		constexpr static auto MULTI_CONDITION = GetIndexValue<MultiCondition, NodeHandle>;
		
		/// [EN] Index of OwnedModule within NodeHandle.
		/// [JP] NodeHandle 内における OwnedModule のインデックス。
		constexpr static auto OWNED_MODULE = GetIndexValue<OwnedModule, NodeHandle>;
		
		/// [EN] Index of AdoptedModule within NodeHandle.
		/// [JP] NodeHandle 内における AdoptedModule のインデックス。
		constexpr static auto ADOPTED_MODULE = GetIndexValue<AdoptedModule, NodeHandle>;

		/**
		* [EN]
		* Default constructor: leaves every field at its in-class default,
		* including a NodeHandle in the Placeholder (no-work) alternative.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: NodeHandle が Placeholder（無処理）の
		* 状態を含め、全フィールドをクラス内デフォルト値のままにする。
		*/
		JobNode() = default;

		/**
		* [EN]
		* Constructs a node using a full TaskParams (name + user data),
		* forwarding the remaining args to construct the NodeHandle.
		* The first four arguments go to JobNodeBase.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 完全な TaskParams（名前 + ユーザーデータ）を用いてノードを
		* 構築し、残りの args を NodeHandle の構築へ転送する。最初の4つの
		* 引数は JobNodeBase へ渡す。
		*/
		template<typename... Args>
		JobNode(NState nstate, EState estate, const TaskParams& params, JobTopology* topology, JobNodeBase* parent, Size joinCounter, Args&&... args) : JobNodeBase(nstate, estate, parent, joinCounter), name_(params.name_), data_(params.data_), topology_(topology), handle_(std::forward<Args>(args)...)
		{
			/// No Code
		}

		/**
		* [EN]
		* Constructs a node with no name or user data (using
		* DefaultTaskParams as a marker), forwarding the remaining
		* args to construct the NodeHandle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 名前もユーザーデータも持たないノードを構築する（マーカーとして
		* DefaultTaskParams を使用）。残りの args を NodeHandle の
		* 構築へ転送する。
		*/
		template<typename... Args>
		JobNode(NState nstate, EState estate, const DefaultTaskParams& defaultTaskParams, JobTopology* topology, JobNodeBase* parent, Size joinCounter, Args&&... args) : JobNodeBase(nstate, estate, parent, joinCounter), topology_(topology), handle_(std::forward<Args>(args)...)
		{
			/// No Code
		}

		/**
		* [EN]
		* Constructs a node using only a string-like name (shorthand for
		* when no user data is needed), forwarding the remaining args
		* to construct the NodeHandle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 文字列型の名前のみを用いてノードを構築する（ユーザーデータが
		* 不要な場合の簡略形）。残りの args を NodeHandle の構築へ
		* 転送する。
		*/
		template<StringLike S, typename... Args>
		JobNode(NState nstate, EState estate, S&& name, JobTopology* topology, JobNodeBase* parent, Size joinCounter, Args&&... args) : JobNodeBase(nstate, estate, parent, joinCounter), name_(std::forward<S>(name)), topology_(topology), handle_(std::forward<Args>(args)...)
		{
			/// No Code
		}

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
		* Returns the number of strong dependencies: predecessors that are
		* not condition nodes, all of which must finish before this node
		* runs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 強い依存関係の数を返す。条件ノードではない先行ノードのことで、
		* このノードの実行前にその全てが終わっている必要がある。
		*/
		Size NumberStrongDependencies()const;

		/**
		* [EN]
		* Returns the number of weak dependencies: predecessors that are
		* condition nodes, any one of which can start this node by
		* choosing it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 弱い依存関係の数を返す。条件ノードである先行ノードのことで、その
		* どれか1つが選ぶだけでこのノードを開始できる。
		*/
		Size NumberWeakDependencies()const;

		/**
		* [EN]
		* Returns this node's display name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードの表示名を返す。
		*/
		const String& Name()const;

	private:
		/// [EN] Display name of this node.
		/// [JP] このノードの表示名。
		String name_;

		/// [EN] Arbitrary user data attached to this node; the node does not own it.
		/// [JP] このノードに付けられた任意のユーザーデータ。ノードはこれを所有しない。
		void* data_ = nullptr;

		/// [EN] The run this node is currently part of; set each time the graph is prepared for a run.
		/// [JP] このノードが今属している実行。グラフを実行に向けて整えるたびに設定される。
		JobTopology* topology_ = nullptr;

		/// [EN] How many entries at the front of edges_ are successors; the rest are predecessors.
		/// [JP] edges_ の先頭から何個が後続か。残りは先行ノード。
		Size numberSuccessors_ = 0;

		/// [EN] Successors in the front numberSuccessors_ entries, predecessors after them; up to 4 are stored inline.
		/// [JP] 先頭 numberSuccessors_ 個が後続、その後ろが先行ノード。4 個まではインラインに格納される。
		HybridArray<JobNode*, 4> edges_;

		/// [EN] The active variant describing what kind of work this node performs.
		/// [JP] このノードが行う処理の種類を表す、現在有効なバリアント値。
		NodeHandle handle_;

		/// [EN] Semaphores to take and give back around this node's run; null when the node uses none, which is the common case.
		/// [JP] このノードの実行前後で取り・返すセマフォ群。使わないノードでは null で、ほとんどのノードがそう。
		std::unique_ptr<Semaphores> semaphores_;

		/**
		* [EN]
		* Returns whether this node's topology or parent node has been
		* cancelled or has failed with an exception, in which case this
		* node is skipped instead of run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードのトポロジーか親ノードが、キャンセルされたか例外で失敗
		* しているかを返す。その場合、このノードは実行されずに飛ばされる。
		*/
		Bool ParentCancelled()const;
		
		/**
		* [EN]
		* Returns whether this node represents a conditional branch
		* (i.e. its handle is SingleCondition or MultiCondition).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが条件分岐を表すかどうか（すなわちハンドルが
		* SingleCondition または MultiCondition であるか）を返す。
		*/
		Bool Conditioner()const;

		/**
		* [EN]
		* Takes every semaphore this node must hold before it runs, in
		* order. If one is not free, this node is parked on it, the ones
		* already taken are given back (their waiters are collected into
		* nodes for rescheduling) and false is returned.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが実行前に持つべきセマフォを、順に全て取る。どれかに
		* 空きが無ければ、このノードをそこで待機させ、既に取った分を返し
		* （その待機者はスケジュールし直すために nodes へ集める）、false を
		* 返す。
		*/
		Bool AcquireAll(HybridArray<JobNode*>& nodes);

		/**
		* [EN]
		* Gives back every semaphore this node releases after running,
		* collecting the tasks that were waiting on them into nodes so
		* they can be rescheduled.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードが実行後に解放するセマフォを全て返し、それらを待って
		* いたタスクを、スケジュールし直せるよう nodes へ集める。
		*/
		void ReleaseAll(HybridArray<JobNode*>& nodes);

		/**
		* [EN]
		* Adds an edge from this node to node: node becomes a successor
		* here, and this node becomes a predecessor there.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードから node へのエッジを足す。こちらでは node が後続に、
		* あちらではこのノードが先行ノードになる。
		*/
		void Precede(JobNode* node);

		/**
		* [EN]
		* Adds the number of strong dependencies to nstate_ and sets the
		* join counter to it, so the node becomes ready when that many
		* predecessors have finished.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 強い依存関係の数を nstate_ に足し、join カウンタをその値にする。
		* その数の先行ノードが終わったところで、このノードは実行可能になる。
		*/
		void SetUpJoinCounter();

		/**
		* [EN]
		* Removes every edge to node from this node's successors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードの後続から、node へのエッジを全て取り除く。
		*/
		void RemoveSuccessors(JobNode* node);

		/**
		* [EN]
		* Removes every edge from node out of this node's predecessors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードの先行ノードから、node からのエッジを全て取り除く。
		*/
		void RemovePredecessors(JobNode* node);
	};

#if SC_ENABLE_TASK_POOL
	/// [EN] Pool type for JobNode; the packed pointer is used where an atomic SynchronizedPointer would not be lock-free.
	/// [JP] JobNode 用のプール型。アトミックな SynchronizedPointer がロックフリーにならない環境では、詰め込んだポインタを使う。
	using NodePool = std::conditional_t
		<
		std::atomic<SynchronizedPointer>::is_always_lock_free,
		ObjectPool<JobNode, SynchronizedPointer>,
		ObjectPool<JobNode, PackedSynchronizedPointer<>>
		>;

	/// [EN] The one pool every executor allocates JobNode objects from when pooling is enabled.
	/// [JP] プールが有効なときに、全てのエグゼキュータが JobNode を確保する唯一のプール。
	inline NodePool nodePool_;
#endif

	/**
	* [EN]
	* Constructs a new JobNode with the given arguments, using the
	* object pool if SC_ENABLE_TASK_POOL is non-zero, or plain new
	* otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定された引数で新しい JobNode を構築する。SC_ENABLE_TASK_POOL
	* が 0 でなければオブジェクトプールを使用し、それ以外の場合は
	* 通常の new を使用する。
	*/
	template<typename... Args>
	__forceinline JobNode* animate(Args&&... args)
	{
#if SC_ENABLE_TASK_POOL
		return nodePool_.Create(std::forward<Args>(args)...);
#else
		return new JobNode(std::forward<Args>(args)...);
#endif
	}

	/**
	* [EN]
	* Releases a JobNode previously created by animate, returning
	* it to the object pool if SC_ENABLE_TASK_POOL is non-zero, or
	* deleting it otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* animate で生成された JobNode を解放する。SC_ENABLE_TASK_POOL
	* が 0 でなければオブジェクトプールへ返却し、それ以外の場合は
	* delete する。
	*/
	__forceinline void recycle(JobNode* node)
	{
#if SC_ENABLE_TASK_POOL
		nodePool_.Recycle(node);
#else
		delete node;
#endif
	}
}