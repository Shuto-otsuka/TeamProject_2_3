#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/JobSystem/FlowBuilder.h>

namespace SeedCore
{
	/**
	* [EN]
	* Owns a named JobGraph together with the queue of JobTopology
	* instances (submitted runs) built from it. This is the top-level
	* entry point users build a graph on via FlowBuilder, then submit to
	* a JobExecutor for execution.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 名前付きの JobGraph と、そこから構築された JobTopology
	* インスタンス（投入された実行）のキューを合わせて所有するクラス。
	* ユーザーが FlowBuilder 経由でグラフを構築し、JobExecutor へ実行を
	* 投入するための、最上位のエントリーポイントとなる。
	*/
	class SEEDCORE_API JobTaskflow :public FlowBuilder
	{
	private:
		/// [EN] The executor queues and runs topologies on graph_ and topologies_ directly.
		/// [JP] エグゼキュータは graph_ と topologies_ を直接使って、トポロジーを積み・実行する。
		friend class JobTopology;
		friend class JobExecutor;
		friend class FlowBuilder;
		friend class Subflow;

	public:
		/**
		* [EN]
		* Constructs an empty taskflow with the given display name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定された表示名で、空のタスクフローを構築する。
		*/
		JobTaskflow(const String& name);

		/**
		* [EN]
		* Constructs an empty taskflow with no name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 名前を持たない、空のタスクフローを構築する。
		*/
		JobTaskflow();

		/**
		* [EN]
		* Move-constructs, transferring rhs's graph, name, and pending topologies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* rhs のグラフ、名前、保留中のトポロジーを移譲してムーブ構築する。
		*/
		JobTaskflow(JobTaskflow&& rhs);

		/**
		* [EN]
		* Move-assigns, transferring rhs's graph, name, and pending topologies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* rhs のグラフ、名前、保留中のトポロジーを移譲してムーブ代入する。
		*/
		JobTaskflow& operator=(JobTaskflow&& rhs);

		/**
		* [EN]
		* Destructor; uses the compiler-generated default (owned members
		* clean themselves up). The taskflow must not be destroyed while
		* a run of it is still in progress.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デストラクタ。コンパイラ生成のデフォルトを使用する（所有する
		* メンバーは自身で後始末される）。実行が進行中のタスクフローを
		* 破棄してはならない。
		*/
		~JobTaskflow() = default;

		/**
		* [EN]
		* Returns the number of tasks (nodes) currently in the graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* グラフ内に現在存在するタスク（ノード）の数を返す。
		*/
		Size NumberTasks()const;

		/**
		* [EN]
		* Returns whether the graph currently has no tasks.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* グラフが現在タスクを持たないかどうかを返す。
		*/
		Bool Empty()const;

		/**
		* [EN]
		* Sets the taskflow's display name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タスクフローの表示名を設定する。
		*/
		void Name(const String& name);

		/**
		* [EN]
		* Returns the taskflow's display name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タスクフローの表示名を返す。
		*/
		const String& Name()const;

		/**
		* [EN]
		* Removes every task from the graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* グラフからすべてのタスクを削除する。
		*/
		void Clear();

		/**
		* [EN]
		* Removes every edge from from to to, on both nodes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* from から to へのエッジを、両方のノードから全て取り除く。
		*/
		void RemoveDependency(JobTask from, JobTask to);

		/**
		* [EN]
		* Returns a reference to the underlying JobGraph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部の JobGraph への参照を返す。
		*/
		JobGraph& Graph();

	private:
		/// [EN] Guards topologies_, which submitting threads and finishing workers change at the same time; also held while moving.
		/// [JP] 投入するスレッドと終わったワーカーが同時に変える topologies_ を守る。ムーブの間も保持する。
		mutable std::mutex mutex_;

		/// [EN] Display name of this taskflow.
		/// [JP] このタスクフローの表示名。
		String name_;

		/// [EN] The graph this taskflow builds and owns.
		/// [JP] このタスクフローが構築・所有するグラフ。
		JobGraph graph_;

		/// [EN] Submitted runs of graph_; the front one is running and the rest wait for it, since they share the graph.
		/// [JP] graph_ の投入済みの実行。先頭が実行中で、残りはそれを待つ。同じグラフを使うため同時には走らない。
		std::queue<ResourceRef<JobTopology>> topologies_;

		/**
		* [EN]
		* Appends a run to the queue and returns the queue size from
		* before the append; 0 means no other run is in progress, so the
		* caller starts this one right away.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 実行を列の末尾へ足し、足す前の列の長さを返す。0 は他に進行中の
		* 実行が無いことを意味し、呼び出し側はこの実行をすぐに始める。
		*/
		Size FetchEnqueue(ResourceRef<JobTopology> topologies);
	};

	/**
	* [EN]
	* Job-system-aware extension of std::future<T>: in addition to the
	* usual future interface, it optionally holds an observing (non
	* owning) reference to the JobTopology so callers can Cancel() the
	* underlying run while it still exists.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョブシステムを意識した std::future<T> の拡張クラス。通常の
	* future インターフェースに加えて、JobTopology への監視（非所有）
	* 参照を任意で保持し、実行がまだ存在する間は呼び出し側が Cancel()
	* できるようにする。
	*/
	template<typename T>
	class JobFuture :public std::future<T>
	{
	private:
		/// [EN] Only the job system creates futures bound to a topology, through the private constructor.
		/// [JP] トポロジーに結びついた future を作るのはジョブシステムだけで、private なコンストラクタを通す。
		friend class JobExecutor;
		friend class Subflow;
		friend class JobPreemptiveRuntime;

	public:
		/**
		* [EN]
		* Default constructor: creates a future with no shared state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: 共有状態を持たない future を生成する。
		*/
		JobFuture() = default;

		/**
		* [EN]
		* Copy construction is disabled, matching std::future's own
		* non-copyable contract.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止されている。std::future 自体のコピー不可契約に
		* 合わせている。
		*/
		JobFuture(const JobFuture&) = delete;

		/**
		* [EN]
		* Move-constructs, transferring the source future's shared state
		* and topology reference (compiler-generated default).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー元の future の共有状態とトポロジー参照を移譲して
		* ムーブ構築する（コンパイラ生成のデフォルト）。
		*/
		JobFuture(JobFuture&&) = default;

		/**
		* [EN]
		* Constructs from a plain std::future<T>, moving it in with no
		* associated topology (Cancel() will report false).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 素の std::future<T> をムーブして構築する。関連付けられた
		* トポロジーは持たない（Cancel() は false を返す）。
		*/
		JobFuture(std::future<T>&& future) :std::future<T>(std::move(future))
		{
			/// No Code
		}

		/**
		* [EN]
		* Copy assignment is disabled, matching std::future's own
		* non-copyable contract.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー代入は禁止されている。std::future 自体のコピー不可契約に
		* 合わせている。
		*/
		JobFuture& operator=(const JobFuture&) = delete;

		/**
		* [EN]
		* Move-assigns, transferring the source future's shared state and
		* topology reference (compiler-generated default).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー元の future の共有状態とトポロジー参照を移譲して
		* ムーブ代入する（コンパイラ生成のデフォルト）。
		*/
		JobFuture& operator=(JobFuture&&) = default;

		/**
		* [EN]
		* Attempts to cancel the run associated with this future by
		* flagging its topology CANCELLED: tasks not yet started are
		* skipped and the run is not repeated. Tasks already running are
		* not interrupted. Returns whether an associated (still alive)
		* topology was found.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この future に関連付けられた実行に対し、そのトポロジーへ
		* CANCELLED フラグを立てることでキャンセルを試みる。まだ始まって
		* いないタスクは飛ばされ、実行は繰り返されない。既に走っている
		* タスクは中断されない。関連付けられた（まだ生存している）トポロジー
		* が見つかったかどうかを返す。
		*/
		Bool Cancel()
		{
			/// [EN] An observing reference tests false once the topology has finished and been destroyed.
			/// [JP] 監視参照は、トポロジーが終わって破棄された後は false と判定される。
			if (topology_)
			{
				/// [EN] Only a flag is set; the workers notice it when they reach the next node.
				/// [JP] 立てるのはフラグだけ。ワーカーは次のノードに進むときにそれに気づく。
				topology_->estate_.fetch_or(JobExceptionState::CANCELLED, std::memory_order_relaxed);
				return true;
			}
			return false;
		}

	private:
		/// [EN] Observing reference to the topology this future's result belongs to; it does not keep the topology alive. Empty if none.
		/// [JP] この future の結果が属するトポロジーへの監視参照。トポロジーを生かし続けはしない。なければ空。
		ResourceRef<JobTopology> topology_;

		/**
		* [EN]
		* Constructs from a std::future<T> and the topology that produces
		* its result, enabling Cancel().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* std::future<T> と、その結果を生成するトポロジーから構築する。
		* これにより Cancel() が使用可能になる。
		*/
		JobFuture(std::future<T>&& future, ResourceRef<JobTopology> pointer) :std::future<T>(std::move(future)), topology_(std::move(pointer))
		{
			/// No Code
		}
	};
}
