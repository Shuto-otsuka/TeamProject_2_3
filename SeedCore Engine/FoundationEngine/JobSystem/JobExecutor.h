#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/JobSystem/JobDeclaretions.h>
#include <FoundationEngine/JobSystem/JobConcept.h>
#include <FoundationEngine/JobSystem/JobWorker.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobNode.h>
#include <FoundationEngine/JobSystem/JobTask.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>
#include <FoundationEngine/JobSystem/JobRuntime.h>
#include <FoundationEngine/JobSystem/WorkerQueue.h>
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/Pool/ObjectPool.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	class JobNodeBase;

	/**
	* [EN]
	* The job system's work-stealing scheduler: owns a pool of
	* JobWorker threads, each with its own Chase-Lev work-stealing
	* queue, plus a set of overflow buffer queues used when a worker's
	* own queue is full. Submitting a JobTaskflow via Run/RunUntil
	* schedules its graph across the worker pool; workers pull from
	* their own queue first, then steal from other workers/buffers when idle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョブシステムのワークスティーリング・スケジューラ。JobWorker
	* スレッドのプールを所有し、各ワーカーは独自の Chase-Lev
	* ワークスティーリングキューを持つ。加えて、ワーカー自身のキューが
	* 満杯になった際に使用する、あふれ用のバッファキュー群も持つ。
	* Run/RunUntil を通じて JobTaskflow を投入すると、そのグラフが
	* ワーカープール全体へスケジューリングされる。ワーカーはまず自身の
	* キューから取り出し、アイドル時には他のワーカー/バッファから
	* 盗み取り（steal）を行う。
	*/
	class SEEDCORE_API JobExecutor
	{
	private:
		/// [EN] Builders and runtimes schedule, corun and spawn work through the executor's private scheduling functions.
		/// [JP] ビルダーとランタイムは、エグゼキュータの private なスケジューリング関数を通じて処理の投入・Corun・生成を行う。
		friend class FlowBuilder;
		friend class JobSubflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;

	public:
		/**
		* [EN]
		* Constructs an executor with n worker threads (defaulting to the
		* hardware concurrency), optionally supplying a custom
		* JobWorkerInterface (e.g. for custom thread naming/affinity)
		* whose hooks run at the start and end of each worker thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* n 個のワーカースレッド（デフォルトはハードウェア並行度）を持つ
		* エグゼキュータを構築する。各ワーカーの生成に使用する、カスタムの
		* JobWorkerInterface（スレッド名やアフィニティのカスタマイズ用など）
		* を任意で指定でき、そのフックが各ワーカースレッドの開始時と終了時に
		* 呼ばれる。
		*/
		explicit JobExecutor(Size n = std::thread::hardware_concurrency(), ResourceRef<JobWorkerInterface> worker = nullptr);

		/**
		* [EN]
		* Waits for all outstanding work to finish, then shuts down and
		* joins every worker thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 未完了のすべての処理を待ってから、全ワーカースレッドを
		* シャットダウンし join する。
		*/
		~JobExecutor();

		/**
		* [EN]
		* Runs taskflow's graph exactly once, returning a future that
		* completes when the run finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* taskflow のグラフをちょうど 1 回実行し、実行完了時に完了する
		* future を返す。
		*/
		JobFuture<void> Run(JobTaskflow& taskflow);

		/**
		* [EN]
		* Runs a moved-in taskflow's graph exactly once, returning a
		* future that completes when the run finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ムーブされた taskflow のグラフをちょうど 1 回実行し、実行完了時に
		* 完了する future を返す。
		*/
		JobFuture<void> Run(JobTaskflow&& taskflow);

		/**
		* [EN]
		* Runs taskflow's graph exactly once, invoking callable after it finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* taskflow のグラフをちょうど 1 回実行し、完了後に callable を
		* 呼び出す。
		*/
		template<typename C>
		JobFuture<void> Run(JobTaskflow& taskflow, C&& callable)
		{
			return RunNumber(taskflow, 1, std::forward<C>(callable));
		}

		/**
		* [EN]
		* Runs a moved-in taskflow's graph exactly once, invoking
		* callable after it finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ムーブされた taskflow のグラフをちょうど 1 回実行し、完了後に
		* callable を呼び出す。
		*/
		template<typename C>
		JobFuture<void> Run(JobTaskflow&& taskflow, C&& callable)
		{
			return RunNumber(std::move(taskflow), 1, std::forward<C>(callable));
		}

		/**
		* [EN]
		* Runs taskflow's graph n times in sequence, returning a future
		* that completes when all n runs finish.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* taskflow のグラフを連続して n 回実行し、n 回すべての実行完了時に
		* 完了する future を返す。
		*/
		JobFuture<void> RunNumber(JobTaskflow& taskflow, Size n);

		/**
		* [EN]
		* Runs a moved-in taskflow's graph n times in sequence, returning
		* a future that completes when all n runs finish.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ムーブされた taskflow のグラフを連続して n 回実行し、n 回すべての
		* 実行完了時に完了する future を返す。
		*/
		JobFuture<void> RunNumber(JobTaskflow&& taskflow, Size n);

		/**
		* [EN]
		* Runs taskflow's graph n times in sequence, invoking callable after the final run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* taskflow のグラフを連続して n 回実行し、最後の実行後に callable
		* を呼び出す。
		*/
		template<typename C>
		JobFuture<void> RunNumber(JobTaskflow& taskflow, Size n, C&& callable)
		{
			/// [EN] The predicate is asked once before the first pass and once after each pass, so it is true after exactly n passes.
			/// [JP] 述語は最初の周の前に1回、各周の後に1回ずつ呼ばれるので、ちょうど n 周の後に true になる。
			return RunUntil(taskflow, [n]() mutable {return n-- == 0;}, std::forward<C>(callable));
		}

		/**
		* [EN]
		* Runs a moved-in taskflow's graph n times in sequence, invoking
		* callable after the final run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ムーブされた taskflow のグラフを連続して n 回実行し、最後の実行後に
		* callable を呼び出す。
		*/
		template<typename C>
		JobFuture<void> RunNumber(JobTaskflow&& taskflow, Size n, C&& callable)
		{
			/// [EN] The predicate is asked once before the first pass and once after each pass, so it is true after exactly n passes.
			/// [JP] 述語は最初の周の前に1回、各周の後に1回ずつ呼ばれるので、ちょうど n 周の後に true になる。
			return RunUntil(std::move(taskflow), [n]() mutable {return n-- == 0;}, std::forward<C>(callable));
		}

		/**
		* [EN]
		* Repeatedly runs taskflow's graph until predicate returns true
		* (checked after each pass), returning a future that completes
		* once the run stops.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* predicate が true を返すまで（各パス後にチェックしながら）
		* taskflow のグラフを繰り返し実行し、実行が停止した時点で完了する
		* future を返す。
		*/
		template<typename P>
		JobFuture<void> RunUntil(JobTaskflow& taskflow, P&& predicate)
		{
			return RunUntil(taskflow, std::forward<P>(predicate), []() {});
		}

		/**
		* [EN]
		* Repeatedly runs a moved-in taskflow's graph until predicate
		* returns true, returning a future that completes once the run stops.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* predicate が true を返すまで、ムーブされた taskflow のグラフを
		* 繰り返し実行し、実行が停止した時点で完了する future を返す。
		*/
		template<typename P>
		JobFuture<void> RunUntil(JobTaskflow&& taskflow, P&& predicate)
		{
			return RunUntil(std::move(taskflow), std::forward<P>(predicate), []() {});
		}

		/**
		* [EN]
		* Repeatedly runs taskflow's graph until predicate returns true,
		* then invokes callable. If the graph is already empty or
		* predicate is already true, runs callable synchronously and
		* returns an already-completed future; otherwise builds a
		* JobTopology, enqueues it on taskflow, and (if it's the first
		* pending run) schedules it immediately.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* predicate が true を返すまで taskflow のグラフを繰り返し実行し、
		* その後 callable を呼び出す。グラフが既に空であるか predicate が
		* 既に true であれば、callable を同期的に実行し、完了済みの future
		* を返す。それ以外の場合は JobTopology を構築して taskflow へ
		* 追加登録し、（それが最初の保留中の実行であれば）直ちにスケジューリング
		* する。
		*/
		template<typename P, typename C>
		JobFuture<void> RunUntil(JobTaskflow& taskflow, P&& predicate, C&& callable)
		{
			/// [EN] Nothing to run: the callback fires right here and the returned future is already complete.
			/// [JP] 実行するものが無いので、コールバックをここで呼び、完了済みの future を返す。
			if (taskflow.Empty() || predicate())
			{
				callable();
				std::promise<void> promise;
				promise.set_value();
				return JobFuture<void>(promise.get_future());
			}

			/// [EN] Counted before anything is scheduled, so WaitForAll cannot slip through while this run is being set up.
			/// [JP] 何かをスケジュールする前に数えておくので、準備中に WaitForAll がすり抜けることはない。
			IncrementTopology();

			ResourceRef<JobTopology> topology = MakeRef<JobTopology>(taskflow, std::forward<P>(predicate), std::forward<C>(callable));

			/// [EN] The future only observes the topology, so it can tell whether the run still exists (e.g. to cancel it).
			/// [JP] future はトポロジーを監視するだけで、実行がまだ存在するか（キャンセルできるか等）を判断できる。
			JobFuture<void> future(topology->promise_.get_future(), MakeObserve(topology));

			/// [EN] Only the first queued run starts now; later ones are started by TearDownTopology when the one before finishes.
			/// [JP] 今始めるのは列の最初の実行だけ。後のものは、前の実行が終わったときに TearDownTopology が始める。
			if (taskflow.FetchEnqueue(topology) == 0)
			{
				SetUpTopology(ThisWorker(), topology.get());
			}

			return future;
		}

		/**
		* [EN]
		* Repeatedly runs a moved-in taskflow's graph until predicate
		* returns true, then invokes callable (see the lvalue overload
		* for the full behavior).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* predicate が true を返すまで、ムーブされた taskflow のグラフを
		* 繰り返し実行し、その後 callable を呼び出す（完全な挙動は
		* 左辺値版のオーバーロードを参照）。
		*/
		template<typename P, typename C>
		JobFuture<void> RunUntil(JobTaskflow&& taskflow, P&& predicate, C&& callable)
		{
			/// [EN] Nothing to run: the callback fires right here and the returned future is already complete.
			/// [JP] 実行するものが無いので、コールバックをここで呼び、完了済みの future を返す。
			if (taskflow.Empty() || predicate())
			{
				callable();
				std::promise<void> promise;
				promise.set_value();
				return JobFuture<void>(promise.get_future());
			}

			/// [EN] Counted before anything is scheduled, so WaitForAll cannot slip through while this run is being set up.
			/// [JP] 何かをスケジュールする前に数えておくので、準備中に WaitForAll がすり抜けることはない。
			IncrementTopology();

			/// [EN] The caller's object may be gone right after this call, so the taskflow is moved into storage the topology owns.
			/// [JP] 呼び出し側のオブジェクトはこの呼び出しの直後に消えうるので、タスクフローはトポロジーが所有する領域へムーブする。
			ResourcePtr<JobTaskflow> ownedTaskflow = MakePtr<JobTaskflow>(std::move(taskflow));
			JobTaskflow& owned = *ownedTaskflow;

			ResourceRef<JobTopology> topology = MakeRef<JobTopology>(owned, std::forward<P>(predicate), std::forward<C>(callable));
			topology->ownedTaskflow_ = std::move(ownedTaskflow);

			/// [EN] The future only observes the topology, so it can tell whether the run still exists (e.g. to cancel it).
			/// [JP] future はトポロジーを監視するだけで、実行がまだ存在するか（キャンセルできるか等）を判断できる。
			JobFuture<void> future(topology->promise_.get_future(), MakeObserve(topology));

			/// [EN] The owned taskflow has no other run queued, so this one always starts right away.
			/// [JP] 所有したタスクフローには他の実行が積まれていないので、この実行は必ずすぐに始まる。
			if (owned.FetchEnqueue(topology) == 0)
			{
				SetUpTopology(ThisWorker(), topology.get());
			}

			return future;
		}

		/**
		* [EN]
		* Runs target's graph synchronously on the calling worker thread
		* (must be called from within a running task), blocking until it completes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼び出し元のワーカースレッド上で target のグラフを同期的に実行し
		* （実行中のタスク内から呼び出す必要がある）、完了するまでブロックする。
		*/
		template<typename T>
		void Corun(T& target)
		{
			/// [EN] Coruning needs a worker to keep running tasks, so a call from outside the pool is an error.
			/// [JP] Corun にはタスクを実行し続けるワーカーが必要なので、プールの外からの呼び出しはエラーにする。
			JobWorker* worker = ThisWorker();
			if (worker == nullptr)
			{
				SC_THROW("Corun は JobExecutor のワーカーから呼び出す必要があります。");
			}

			/// [EN] A local anchor stands in as the parent node, so its counter tells when every node of the graph is done.
			/// [JP] ローカルのアンカーを親ノード代わりにし、そのカウンタで graph の全ノードが終わったことを知る。
			JobNodeBase anchor;
			CorunGraph(*worker, RetrieveGraph(target), nullptr, &anchor);
		}

		/**
		* [EN]
		* Helps the calling worker thread process other scheduled work
		* until predicate returns true (must be called from within a running task).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* predicate が true を返すまで、呼び出し元のワーカースレッドに
		* 他のスケジュール済み処理を手伝わせる（実行中のタスク内から
		* 呼び出す必要がある）。
		*/
		template<typename P>
		void CorunUntil(P&& predicate)
		{
			/// [EN] Coruning needs a worker to keep running tasks, so a call from outside the pool is an error.
			/// [JP] Corun にはタスクを実行し続けるワーカーが必要なので、プールの外からの呼び出しはエラーにする。
			JobWorker* worker = ThisWorker();
			if (worker == nullptr)
			{
				SC_THROW("CorunUntil は JobExecutor のワーカーから呼び出す必要があります。");
			}

			CorunUntil(*worker, std::forward<P>(predicate));
		}

		/**
		* [EN]
		* Blocks the calling thread until every currently outstanding
		* topology (across all taskflows run on this executor) has finished.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このエグゼキュータで実行された全タスクフローにわたる、現在未完了の
		* すべてのトポロジーが完了するまで、呼び出し元のスレッドをブロックする。
		*/
		void WaitForAll();

		/**
		* [EN]
		* Returns the number of worker threads owned by this executor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このエグゼキュータが所有するワーカースレッドの数を返す。
		*/
		Size NumberWorkers()const noexcept;

		/**
		* [EN]
		* Returns the number of threads currently parked waiting for work.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在、処理待ちでパークされているスレッドの数を返す。
		*/
		Size NumberWaiters()const noexcept;

		/**
		* [EN]
		* Returns the total number of stealable queues (worker queues
		* plus overflow buffer queues).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 盗み取り可能なキューの総数（ワーカーキューとあふれ用バッファ
		* キューの合計）を返す。
		*/
		Size NumberQueues()const noexcept;

		/**
		* [EN]
		* Returns the number of topologies currently outstanding across the executor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エグゼキュータ全体で現在未完了となっているトポロジーの数を返す。
		*/
		Size NumberTopologies()const noexcept;

		/**
		* [EN]
		* Returns the JobWorker associated with the calling thread, or
		* nullptr if the calling thread is not one of this executor's workers.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼び出し元のスレッドに関連付けられた JobWorker を返す。呼び出し元
		* スレッドがこのエグゼキュータのワーカーでなければ nullptr を返す。
		*/
		JobWorker* ThisWorker();

		/**
		* [EN]
		* Returns the index of the calling thread's worker, or -1 if the
		* calling thread is not one of this executor's workers.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼び出し元のスレッドのワーカーインデックスを返す。呼び出し元
		* スレッドがこのエグゼキュータのワーカーでなければ -1 を返す。
		*/
		Int ThisWorkerID()const;

	private:
		/**
		* [EN]
		* An overflow queue (used when a worker's own queue is full or
		* the pusher isn't a worker) guarded by its own mutex.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* あふれ用のキュー（ワーカー自身のキューが満杯の場合や、投入元が
		* ワーカーでない場合に使用される）。専用のミューテックスで保護される。
		*/
		struct Buffer
		{
			/// [EN] Serializes pushes, since any thread may push here while the queue itself allows only one pusher.
			/// [JP] push を1つずつにする。ここへは任意のスレッドが push するが、キュー自体は push 側を1つしか許さないため。
			std::mutex mutex_;

			/// [EN] The underlying unbounded work-stealing queue; stealing from it needs no lock.
			/// [JP] 内部の無制限ワークスティーリングキュー。ここからの盗み取りにロックは要らない。
			UnboundedWorkerQueue<JobNode*> queue_;
		};

		/// [EN] The pool of worker threads owned by this executor.
		/// [JP] このエグゼキュータが所有するワーカースレッドのプール。
		DynamicArray<JobWorker> workers_;

		/// [EN] Overflow queues (bit_width of the worker count), used when a worker's own queue is full or the pusher is not a worker.
		/// [JP] あふれ用キュー（ワーカー数の bit_width 個）。ワーカー自身のキューが満杯のときや、投入元がワーカーでないときに使う。
		DynamicArray<Buffer> buffers_;

		/// [EN] Eventcount notifier used to wake parked workers when new work becomes available.
		/// [JP] 新しい処理が発生した際にパーク中のワーカーを起床させる、eventcount notifier。
		alignas(SC_CACHELINE_SIZE)DefaultNotifier notifier_;

		/// [EN] Count of topologies currently outstanding across the executor.
		/// [JP] エグゼキュータ全体で現在未完了となっているトポロジーの数。
		alignas(SC_CACHELINE_SIZE)std::atomic<Size> numberTopologies_{ 0 };

		/// [EN] Maps a std::thread::id to the owning JobWorker, so ThisWorker() can identify the calling thread.
		/// [JP] std::thread::id をそれを所有する JobWorker へ対応付ける。ThisWorker() が呼び出し元スレッドを特定するために使う。
		std::unordered_map<std::thread::id, JobWorker*> thread2Worker_;

	private:
		/**
		* [EN]
		* Waits for every outstanding topology, then signals every worker
		* to stop and joins their threads.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 未完了のトポロジーを全て待ってから、全ワーカーへ停止を通知し、
		* それらのスレッドを join する。
		*/
		void Shutdown();

		/**
		* [EN]
		* Starts n worker threads, each running the exploit/wait
		* scheduling loop with worker's prologue and epilogue around it,
		* and registers each thread in thread2Worker_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* n 個のワーカースレッドを起動する。各スレッドは worker の前処理と
		* 後処理に挟まれた、消化/待機のスケジューリングループを回す。各
		* スレッドは thread2Worker_ に登録する。
		*/
		void Spawn(Size n, ResourceRef<JobWorkerInterface> worker);

		/**
		* [EN]
		* Runs cache, then keeps popping and running nodes from worker's
		* own queue until it is empty; cache is left null.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* cache を実行し、その後 worker 自身のキューが空になるまでノードを
		* 取り出して実行し続ける。終わったとき cache は null になっている。
		*/
		void ExploitTask(JobWorker& worker, JobNode*& cache);

		/**
		* [EN]
		* Tries to steal one node from another worker's queue or a buffer
		* into cache (it does not run it). Returns false only when worker
		* has been told to stop; true otherwise, whether or not anything
		* was stolen.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 他のワーカーのキューかバッファから、ノードを1つ cache へ盗み取る
		* （実行はしない）。false を返すのは worker に停止が指示されたとき
		* だけで、それ以外は盗めたかどうかに関係なく true を返す。
		*/
		Bool ExploreTask(JobWorker& worker, JobNode*& cache);

		/**
		* [EN]
		* Pushes cache onto worker's own queue (or a buffer if that queue
		* is full) and wakes one parked worker to come and steal it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* cache を worker 自身のキュー（満杯であればバッファ）へ push し、
		* それを盗みに来るよう、眠っているワーカーを1つ起こす。
		*/
		void Schedule(JobWorker& worker, JobNode*& cache);

		/**
		* [EN]
		* Schedules cache from a thread that is not a worker, by putting
		* it in a buffer and waking one parked worker.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワーカーではないスレッドから cache をスケジューリングする。
		* バッファへ入れ、眠っているワーカーを1つ起こす。
		*/
		void Schedule(JobNode*& cache);

		/**
		* [EN]
		* Prepares every node of graph to run under topology/parent and
		* schedules the ones with no predecessor, adding their count to
		* parent's join counter.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* graph の全ノードを topology/parent のもとで実行できるよう整え、
		* 先行ノードを持たないものをスケジューリングする。その数を parent
		* の join カウンタへ足す。
		*/
		void ScheduleGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent);

		/**
		* [EN]
		* Puts node into one of the overflow buffer queues.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node を、あふれ用バッファキューのどれか1つへ入れる。
		*/
		void Spill(JobNode* node);

		/**
		* [EN]
		* Starts one run of topology's graph: prepares every node, sets
		* the topology's join counter to the number of source nodes and
		* schedules them. worker is null when called from outside the pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* topology のグラフの1回分の実行を始める。全ノードを整え、トポロジー
		* の join カウンタをソースノードの数にし、それらをスケジューリング
		* する。プールの外から呼ばれた場合、worker は null になる。
		*/
		void SetUpTopology(JobWorker* worker, JobTopology* topology);

		/**
		* [EN]
		* Resets every node of graph for a new run under topology/parent
		* and moves the source nodes (no predecessor) to the front of the
		* graph. Returns how many source nodes there are.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* graph の全ノードを、topology/parent のもとでの新しい実行に向けて
		* 初期化し、ソースノード（先行ノードなし）をグラフの先頭へ寄せる。
		* ソースノードの数を返す。
		*/
		Size SetUpGraph(JobGraph& graph, JobTopology* topology, JobNodeBase* parent);

		/**
		* [EN]
		* Called when every node of topology's run has finished. Starts
		* another pass if the topology is not cancelled and its predicate
		* is still false; otherwise finishes the topology and moves on to
		* the next run queued on the same taskflow, or notifies the
		* parent node that waited on it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* topology の実行で全ノードが終わったときに呼ばれる。キャンセル
		* されておらず述語がまだ false なら、もう1周実行する。そうでなければ
		* トポロジーを終わらせ、同じタスクフローに積まれた次の実行へ進むか、
		* これを待っていた親ノードへ知らせる。
		*/
		void TearDownTopology(JobWorker& worker, JobTopology* topology, JobNode*& cache);

		/**
		* [EN]
		* Accounts for a finished node on its parent: decrements the
		* parent's join counter and, if this was the last outstanding
		* child, either finishes the topology or resumes the suspended
		* parent node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 終わったノードを親の側に反映する。親の join カウンタを減らし、
		* これが最後の子だった場合、トポロジーを終わらせるか、中断している
		* 親ノードを再開する。
		*/
		void TearDownNonasync(JobWorker& worker, JobNode* node, JobNode*& cache);

		/**
		* [EN]
		* Tears node down without running it, as done for a node whose
		* run was cancelled.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node を実行せずに片付ける。実行がキャンセルされたノードに対して
		* 行う。
		*/
		void TearDownInvoke(JobWorker& worker, JobNode* node, JobNode*& cache);

		/**
		* [EN]
		* Increments the executor-wide outstanding topology count.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エグゼキュータ全体の未完了トポロジー数をインクリメントする。
		*/
		void IncrementTopology();

		/**
		* [EN]
		* Decrements the executor-wide outstanding topology count, waking
		* any thread blocked in WaitForAll if it just reached zero.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エグゼキュータ全体の未完了トポロジー数をデクリメントし、それが
		* ちょうど 0 になった場合は WaitForAll でブロック中のスレッドを
		* 起床させる。
		*/
		void DecrementTopology();

		/**
		* [EN]
		* Runs node according to its NodeHandle alternative, then releases
		* its semaphores, makes ready the successors it unblocked and
		* tears it down. One newly ready successor is run next in the same
		* call, in a loop rather than by recursion.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node をその NodeHandle の選択肢に従って実行し、その後セマフォを
		* 解放し、待ちが解けた後続ノードを実行可能にし、node を片付ける。
		* 新たに実行可能になった後続の1つは、再帰ではなくループで、同じ
		* 呼び出しの中で続けて実行する。
		*/
		void Invoke(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Invokes a Static task's callable.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Static タスクの呼び出し可能オブジェクトを実行する。
		*/
		void InvokeStaticTask(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Invokes a NonpreemptiveRuntime task's callable, passing it a
		* JobNonpreemptiveRuntime handle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* NonpreemptiveRuntime タスクの呼び出し可能オブジェクトを、
		* JobNonpreemptiveRuntime ハンドルを渡して実行する。
		*/
		void InvokeNonpreemptiveRuntimeTask(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Invokes a SingleCondition task's callable and replaces conds
		* with the single successor index it returns.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SingleCondition タスクの呼び出し可能オブジェクトを実行し、conds
		* をそれが返した単一の後続インデックスで置き換える。
		*/
		void InvokeSingleConditionTask(JobWorker& worker, JobNode* node, HybridArray<Int>& conds);

		/**
		* [EN]
		* Invokes a MultiCondition task's callable and replaces conds with
		* every successor index it returns.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* MultiCondition タスクの呼び出し可能オブジェクトを実行し、conds を
		* それが返した全ての後続インデックスで置き換える。
		*/
		void InvokeMultiConditionTask(JobWorker& worker, JobNode* node, HybridArray<Int>& conds);

		/**
		* [EN]
		* On first entry, creates the JobSubflow builder for the node and
		* schedules the subgraph, suspending the node (returns true). On
		* the resuming call, clears the subgraph unless it is retained and
		* returns false so the node finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 初回はノード用の JobSubflow ビルダーを作ってサブグラフをスケジュール
		* し、ノードを中断する（true を返す）。再開時は、保持指定が無ければ
		* サブグラフを消し、ノードが終わるよう false を返す。
		*/
		Bool InvokeSubflowTask(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Handles an OwnedModule task by delegating to
		* InvokeModuleTaskImplementation with the externally-owned graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 外部が所有するグラフを渡して InvokeModuleTaskImplementation へ
		* 委譲することで、OwnedModule タスクを処理する。
		*/
		Bool InvokeOwnedModuleTask(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Handles an AdoptedModule task by delegating to
		* InvokeModuleTaskImplementation with the adopted (owned) graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 養子化（所有）されたグラフを渡して InvokeModuleTaskImplementation
		* へ委譲することで、AdoptedModule タスクを処理する。
		*/
		Bool InvokeAdoptedModuleTask(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Shared implementation for OwnedModule/AdoptedModule: on first
		* entry schedules graph as node's children and suspends the node
		* (returns true); on the resuming call returns false so the node
		* finishes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OwnedModule/AdoptedModule の共通実装。初回は graph を node の子と
		* してスケジューリングしてノードを中断する（true を返す）。再開時は
		* ノードが終わるよう false を返す。
		*/
		Bool InvokeModuleTaskImplementation(JobWorker& worker, JobNode* node, JobGraph& graph);

		/**
		* [EN]
		* Invokes a PreemptiveRuntime task's callable, passing it a
		* JobPreemptiveRuntime through which it can spawn more tasks.
		* Returns whether the node was suspended to wait for them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* PreemptiveRuntime タスクの呼び出し可能オブジェクトを、さらに
		* タスクを生成できる JobPreemptiveRuntime を渡して実行する。それらを
		* 待つためにノードを中断したかどうかを返す。
		*/
		Bool InvokePreemptiveRuntimeTask(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Calls function(runtime) on first entry; if tasks it spawned are
		* still running when it returns, suspends the node (returns true)
		* until the last of them resumes it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 初回は function(runtime) を呼ぶ。戻った時点で生成したタスクが
		* まだ走っていれば、それらの最後の1つが再開させるまでノードを中断
		* する（true を返す）。
		*/
		Bool InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&)>& function);

		/**
		* [EN]
		* Variant for callables that also receive a "resumed" flag: calls
		* function(runtime, false) first, and function(runtime, true) once
		* every task it spawned has finished.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 「再開か」のフラグも受け取る処理向けの版。最初に
		* function(runtime, false) を呼び、生成したタスクが全て終わった時点で
		* function(runtime, true) を呼ぶ。
		*/
		Bool InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&, Bool)>& function);

		/**
		* [EN]
		* Records the exception currently in flight: flags node and its
		* ancestors up to the nearest explicit anchor with EXCEPTION, and
		* stores the exception on that explicit anchor, or else on the
		* nearest implicit anchor, or else on node itself.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今投げられている例外を記録する。node から最も近い明示アンカーの
		* 手前までの祖先に EXCEPTION を立て、例外はその明示アンカーに、
		* 無ければ最も近い暗黙アンカーに、それも無ければ node 自身に格納する。
		*/
		void ProcessException(JobWorker& worker, JobNode* node);

		/**
		* [EN]
		* Makes node the next one to run on this thread, first scheduling
		* whatever was previously in cache so other workers can take it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node を、このスレッドで次に実行するものにする。その前に、それまで
		* cache にあったものをスケジューリングし、他のワーカーが取れるように
		* する。
		*/
		void UpdateCache(JobWorker& worker, JobNode*& cache, JobNode* node);

		/**
		* [EN]
		* Schedules graph under topology/parent on worker and keeps worker
		* running tasks (its own or stolen) until every node of graph has
		* finished, so the calling task blocks without idling its thread.
		* parent is explicitly anchored meanwhile, and an exception thrown
		* inside the graph is rethrown to the caller afterwards.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* worker 上で graph を topology/parent のもとでスケジューリングし、
		* graph の全ノードが終わるまで worker にタスク（自分のものでも盗んだ
		* ものでも）を実行させ続ける。呼び出し元のタスクは、スレッドを遊ば
		* せずに待てる。その間 parent を明示アンカーにし、graph の中で投げ
		* られた例外は終わった後に呼び出し側へ投げ直す。
		*/
		void CorunGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent);

		/**
		* [EN]
		* Finds the next node for worker: steals one if possible, and
		* otherwise parks the thread on the notifier until new work is
		* published. Returns true with a node in cache, or false once
		* worker has been told to stop.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* worker の次のノードを探す。盗めるなら盗み、無ければ新しい仕事が
		* 公開されるまで notifier でスレッドを眠らせる。cache にノードを
		* 入れて true を返すか、worker に停止が指示されていれば false を返す。
		*/
		Bool WaitForTask(JobWorker& worker, JobNode*& cache);

		/**
		* [EN]
		* Keeps worker running tasks until stopPredicate returns true:
		* pops from its own queue first, and steals from the other queues
		* when its own is empty. Lets a task wait for other work without
		* leaving its thread idle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* stopPredicate が true を返すまで、worker にタスクを実行させ続ける。
		* まず自分のキューから取り、空なら他のキューから盗む。タスクが他の
		* 処理を待つ間も、そのスレッドを遊ばせない。
		*/
		template<typename P>
		void CorunUntil(JobWorker& worker, P&& stopPredicate)
		{
			/// [EN] After about two full rounds of failed steals the thread yields between attempts.
			/// [JP] 盗み取りにおよそ2周分失敗したら、試行の合間にタイムスライスを譲る。
			const Size MAX_VICTIM = NumberQueues();
			const Size MAX_STEALS = ((MAX_VICTIM + 1) << 1);

		exploit:

			/// [EN] The predicate is re-checked after every task, since any of them may be the one it waits for.
			/// [JP] どのタスクが待っている相手かは分からないので、1つ実行するごとに述語を確かめ直す。
			while (!stopPredicate())
			{
				if (auto t = worker.wsq_.pop();t)
				{
					Invoke(worker, t);
				}
				else
				{
					Size numberSteals = 0;
					Size victim = worker.stickyVictim_;

				explore:

					/// [EN] Victim indices below the worker count are worker queues; the rest map onto the buffers.
					/// [JP] ワーカー数未満の番号はワーカーのキュー、それ以降はバッファに対応する。
					t = (victim < workers_.size()) ? workers_[victim].wsq_.steal() : buffers_[victim - workers_.size()].queue_.steal();

					/// [EN] A successful steal goes back to the own queue first, since running t may have pushed new work there.
					/// [JP] 盗めたら、まず自分のキューへ戻る。t の実行でそこに新しい仕事が積まれている可能性があるため。
					if (t)
					{
						Invoke(worker, t);
						worker.stickyVictim_ = victim;
						goto exploit;
					}
					else if (!stopPredicate())
					{
						/// [EN] Unlike the idle loop this never parks: the waiting task must notice the predicate as soon as it turns true.
						/// [JP] 待機中のループと違って決して眠らない。待っているタスクは、述語が true になったらすぐ気づく必要がある。
						if (++numberSteals > MAX_STEALS)
						{
							std::this_thread::yield();
						}
						victim = worker.rdgen_() % MAX_VICTIM;
						goto explore;
					}
					else
					{
						break;
					}
				}
			}
		}

		/**
		* [EN]
		* Bulk-pushes n items starting at first onto worker's own queue,
		* spilling whatever doesn't fit, then wakes up to n parked workers.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* first から始まる n 個の要素を worker 自身のキューへ一括 push し、
		* 収まらなかった分をあふれさせた後、眠っているワーカーを最大 n 個
		* 起こす。
		*/
		template<typename I>
		void BulkSchedule(JobWorker& worker, I first, Size n)
		{
			if (n == 0)
			{
				return;
			}

			/// [EN] try_bulk_push advances first past what it pushed, so the spill starts at the first item that did not fit.
			/// [JP] try_bulk_push は push した分だけ first を進めるので、あふれ処理は入りきらなかった最初の要素から始まる。
			if (auto num = worker.wsq_.try_bulk_push(first, n);num != n)
			{
				BulkSpill(first, n - num);
			}

			/// [EN] One wakeup per item, so each can be stolen by a different worker.
			/// [JP] 要素1つにつき1回起こし、それぞれを別のワーカーが盗めるようにする。
			notifier_.notify_count(n);
		}

		/**
		* [EN]
		* Bulk-schedules n items starting at first from a non-worker
		* thread by spilling all of them, then wakes up to n parked workers.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワーカーでないスレッドから、first で始まる n 個の要素をすべて
		* あふれさせることで一括スケジューリングし、眠っているワーカーを
		* 最大 n 個起こす。
		*/
		template<typename I>
		void BulkSchedule(I first, Size n)
		{
			if (n == 0)
			{
				return;
			}

			BulkSpill(first, n);
			notifier_.notify_count(n);
		}

		/**
		* [EN]
		* Bulk-pushes n items starting at first into a single overflow
		* buffer chosen by hashing the first item.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭要素をハッシュして選んだ単一のあふれ用バッファへ、first から
		* 始まる n 個の要素を一括 push する。
		*/
		template<typename I>
		void BulkSpill(I first, Size n)
		{
			/// [EN] Multiplicative (Knuth) hashing of the first node's address spreads batches from different graphs across the buffers.
			/// [JP] 先頭ノードのアドレスを乗算（Knuth）ハッシュすることで、別々のグラフから来たまとまりをバッファへ散らす。
			auto buffer = ((reinterpret_cast<uintptr_t>(*first) * 2654435761ULL) >> 32) % buffers_.size();
			std::scoped_lock lock(buffers_[buffer].mutex_);
			buffers_[buffer].queue_.bulk_push(first, n);
		}

		/**
		* [EN]
		* Bulk-pushes n items starting at first, distributing them
		* round-robin across every overflow buffer starting from a
		* hash-chosen offset (spreads load more evenly than BulkSpill).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* first から始まる n 個の要素を、ハッシュで選んだオフセットから
		* 開始して全あふれ用バッファへラウンドロビンで分配し一括 push する
		* （BulkSpill よりも負荷をより均等に分散する）。
		*/
		template<typename I>
		void BulkSpillRoundRobin(I first, Size n)
		{
			/// [EN] Each buffer receives at most ceil(n / buffer count) items, starting from a hash-chosen buffer.
			/// [JP] ハッシュで選んだバッファから始めて、各バッファに最大 ceil(n / バッファ数) 個ずつ入れる。
			const Size buffer = buffers_.size();
			const Size start = ((reinterpret_cast<uintptr_t>(*first) * 2654435761ULL) >> 32) % buffer;
			const Size perBuffer = (n + buffer - 1) / buffer;
			Size remaining = n;
			for (Size index = 0;index < buffer && remaining>0;++index)
			{
				Size b = (start + index) % buffer;
				Size chunk = Min(perBuffer, remaining);

				/// [EN] Each buffer is locked only for its own chunk.
				/// [JP] ロックするのは、そのバッファへ入れる分の間だけ。
				{
					std::scoped_lock lock(buffers_[b].mutex_);
					buffers_[b].queue_.bulk_push(first, chunk);
				}

				remaining -= chunk;
			}
		}

		/**
		* [EN]
		* Fixed-capacity (N) variant of UpdateCache: buffers node into
		* array instead of scheduling immediately, flushing the batch via
		* BulkSchedule once it reaches N entries.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* UpdateCache の固定容量（N）版: node を即座にスケジューリングせず
		* array へ溜め込み、N 個に達した時点で BulkSchedule によって
		* バッチを送出する。
		*/
		template<Size N>
		void BulkUpdateCache(JobWorker& worker, JobNode*& cache, JobNode* node, StaticArray<JobNode*, N>& array, Size& n)
		{
			/// [EN] The previous cache goes into the batch instead of being scheduled one by one, saving a notify per node.
			/// [JP] それまでの cache は1つずつスケジュールせずにまとめへ入れ、ノードごとの通知を省く。
			if (cache)
			{
				array[n++] = cache;
				if (n == N)
				{
					BulkSchedule(worker, array.begin(), n);
					n = 0;
				}
			}
			cache = node;
		}
	};
}
