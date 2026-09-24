#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>
#include <FoundationEngine/JobSystem/JobTopology.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs an executor with n worker threads, one bounded queue per
	* worker plus bit_width(n) overflow buffers, then starts the threads.
	* Throws if n is 0. If spawning fails partway, the threads already
	* started are shut down before the exception is rethrown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* n 個のワーカースレッドを持つエグゼキュータを構築する。ワーカーごとに
	* 有界キューを1つ、加えて bit_width(n) 個のあふれ用バッファを持ち、
	* その後スレッドを起動する。n が 0 なら例外を投げる。起動の途中で
	* 失敗した場合は、起動済みのスレッドを止めてから例外を投げ直す。
	*/
	JobExecutor::JobExecutor(Size n, ResourceRef<JobWorkerInterface> worker) :workers_(n), buffers_(std::bit_width(n)), notifier_(n)
	{
		/// [EN] An executor needs at least one worker, and the steal loops rely on there being at least two queues.
		/// [JP] エグゼキュータには少なくとも1つのワーカーが必要で、盗み取りのループはキューが2つ以上あることを前提にしている。
		if (n == 0)
		{
			SC_THROW("JobExecutor には少なくとも1つのワーカーが必要です。");
		}

#if !SC_DISABLE_EXCEPTION_HANDLING
		try
		{
#endif
			Spawn(n, std::move(worker));
#if !SC_DISABLE_EXCEPTION_HANDLING
		}
		catch (...)
		{
			/// [EN] Workers that did start must be joined, or their std::thread destructors would terminate the program.
			/// [JP] 起動できたワーカーは join しておく。そうしないと std::thread のデストラクタがプログラムを終了させる。
			Shutdown();
			std::rethrow_exception(std::current_exception());
		}
#endif
	}

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
	JobExecutor::~JobExecutor()
	{
		Shutdown();
	}

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
	JobFuture<void> JobExecutor::Run(JobTaskflow& taskflow)
	{
		return RunNumber(taskflow, 1, []() {});
	}

	/**
	* [EN]
	* Runs a moved-in taskflow's graph exactly once, returning a future
	* that completes when the run finishes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ムーブされた taskflow のグラフをちょうど 1 回実行し、実行完了時に
	* 完了する future を返す。
	*/
	JobFuture<void> JobExecutor::Run(JobTaskflow&& taskflow)
	{
		return RunNumber(std::move(taskflow), 1, []() {});
	}

	/**
	* [EN]
	* Runs taskflow's graph n times in sequence, returning a future that
	* completes when all n runs finish.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* taskflow のグラフを連続して n 回実行し、n 回すべての実行完了時に
	* 完了する future を返す。
	*/
	JobFuture<void> JobExecutor::RunNumber(JobTaskflow& taskflow, Size n)
	{
		return RunNumber(taskflow, n, []() {});
	}

	/**
	* [EN]
	* Runs a moved-in taskflow's graph n times in sequence, returning a
	* future that completes when all n runs finish.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ムーブされた taskflow のグラフを連続して n 回実行し、n 回すべての
	* 実行完了時に完了する future を返す。
	*/
	JobFuture<void> JobExecutor::RunNumber(JobTaskflow&& taskflow, Size n)
	{
		return RunNumber(std::move(taskflow), n, []() {});
	}

	/**
	* [EN]
	* Blocks the calling thread until every currently outstanding
	* topology has finished.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在未完了のすべてのトポロジーが完了するまで、呼び出し元の
	* スレッドをブロックする。
	*/
	void JobExecutor::WaitForAll()
	{
		/// [EN] DecrementTopology notifies this atomic when the count reaches zero, so the thread sleeps instead of spinning.
		/// [JP] 数が 0 になると DecrementTopology がこのアトミックへ通知するので、回り続けずに眠って待てる。
		Size n = numberTopologies_.load(std::memory_order_acquire);
		while (n != 0)
		{
			/// [EN] wait() also returns when the count changes to anything else, so the value is read again and re-checked.
			/// [JP] wait() は 0 以外への変化でも戻るため、値を読み直して確かめ直す。
			numberTopologies_.wait(n, std::memory_order_acquire);
			n = numberTopologies_.load(std::memory_order_acquire);
		}
	}

	/**
	* [EN]
	* Returns the number of worker threads owned by this executor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このエグゼキュータが所有するワーカースレッドの数を返す。
	*/
	Size JobExecutor::NumberWorkers()const noexcept
	{
		return workers_.size();
	}

	/**
	* [EN]
	* Returns the number of threads currently parked waiting for work.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在、処理待ちでパークされているスレッドの数を返す。
	*/
	Size JobExecutor::NumberWaiters()const noexcept
	{
		return notifier_.count();
	}

	/**
	* [EN]
	* Returns the total number of stealable queues (worker queues plus
	* overflow buffer queues).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 盗み取り可能なキューの総数（ワーカーキューとあふれ用バッファ
	* キューの合計）を返す。
	*/
	Size JobExecutor::NumberQueues()const noexcept
	{
		return workers_.size() + buffers_.size();
	}

	/**
	* [EN]
	* Returns the number of topologies currently outstanding across the executor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エグゼキュータ全体で現在未完了となっているトポロジーの数を返す。
	*/
	Size JobExecutor::NumberTopologies()const noexcept
	{
		return numberTopologies_.load(std::memory_order_relaxed);
	}

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
	JobWorker* JobExecutor::ThisWorker()
	{
		/// [EN] The map is filled once in Spawn and never changes afterwards, so it is read without a lock.
		/// [JP] この表は Spawn で一度埋めた後は変わらないので、ロックなしで読む。
		auto iterator = thread2Worker_.find(std::this_thread::get_id());
		return iterator == thread2Worker_.end() ? nullptr : iterator->second;
	}

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
	Int JobExecutor::ThisWorkerID()const
	{
		auto index = thread2Worker_.find(std::this_thread::get_id());
		return index == thread2Worker_.end() ? -1 : static_cast<Int>(index->second->id_);
	}

	/**
	* [EN]
	* Waits for every outstanding topology, then signals every worker to
	* stop and joins their threads.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 未完了のトポロジーを全て待ってから、全ワーカーへ停止を通知し、
	* それらのスレッドを join する。
	*/
	void JobExecutor::Shutdown()
	{
		/// [EN] Stopping only after the work is done means no scheduled task is ever abandoned.
		/// [JP] 仕事が終わってから止めるので、スケジュール済みのタスクが置き去りになることはない。
		WaitForAll();

		/// [EN] Each worker checks its own flag before parking, so every flag is set before anyone is woken.
		/// [JP] 各ワーカーは眠る前に自分のフラグを見るので、起こす前に全員のフラグを立てておく。
		for (Size index = 0;index < workers_.size();++index)
		{
			workers_[index].done_.test_and_set(std::memory_order_relaxed);
		}

		/// [EN] Parked workers wake up, see their flag and leave the scheduling loop.
		/// [JP] 眠っているワーカーが起き、フラグを見てスケジューリングループを抜ける。
		notifier_.notify_all();

		/// [EN] A thread that was never started (a failed Spawn) is not joinable and is skipped.
		/// [JP] 起動されなかったスレッド（Spawn の失敗時）は join できないので飛ばす。
		for (JobWorker& worker : workers_)
		{
			if (worker.thread_.joinable())
			{
				worker.thread_.join();
			}
		}
	}

	/**
	* [EN]
	* Starts n worker threads, each running the exploit/wait scheduling
	* loop with workerInterface's prologue and epilogue around it, and
	* registers each thread in thread2Worker_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* n 個のワーカースレッドを起動する。各スレッドは workerInterface の
	* 前処理と後処理に挟まれた、消化/待機のスケジューリングループを回す。
	* 各スレッドは thread2Worker_ に登録する。
	*/
	void JobExecutor::Spawn(Size n, ResourceRef<JobWorkerInterface> workerInterface)
	{
		for (Size id = 0;id < n;++id)
		{
			workers_[id].thread_ = std::thread([&, id, workerInterface]()
				{
					auto& worker = workers_[id];

					/// [EN] A worker first tries to steal from itself, which quickly moves it on to a random victim.
					/// [JP] ワーカーは最初に自分自身を盗み取り対象とし、すぐにランダムな対象へ移っていく。
					worker.id_ = id;
					worker.stickyVictim_ = id;

					/// [EN] Seeding from the thread id gives every worker a different victim sequence.
					/// [JP] スレッド ID から種を作ることで、ワーカーごとに異なる対象の並びになる。
					worker.rdgen_.Seed(static_cast<Uint32>(std::hash<std::thread::id>()(std::this_thread::get_id())));

					if (workerInterface)
					{
						workerInterface->SchedulerPrologue(worker);
					}

					/// [EN] t carries the next node to run between exploiting and waiting.
					/// [JP] t は、消化と待機の間で次に実行するノードを受け渡す。
					JobNode* t = nullptr;

					/// [EN] Filled only when the loop is left by an exception, and handed to the epilogue.
					/// [JP] ループが例外で抜けたときだけ埋まり、後処理へ渡される。
					std::exception_ptr ptr = nullptr;

#if !SC_DISABLE_EXCEPTION_HANDLING
					try
					{
#endif
						/// [EN] Run everything reachable from t, then find more work; leave only when WaitForTask reports shutdown.
						/// [JP] t から辿れるものを全て実行し、次の仕事を探す。WaitForTask が終了を告げたときだけ抜ける。
						while (1)
						{
							ExploitTask(worker, t);

							if (WaitForTask(worker, t) == false)
							{
								break;
							}
						}
#if !SC_DISABLE_EXCEPTION_HANDLING
					}
					catch (...)
					{
						ptr = std::current_exception();
					}
#endif

					if (workerInterface)
					{
						workerInterface->SchedulerEpilogue(worker, ptr);
					}
			});

			/// [EN] Registered from the constructing thread, before any task can be submitted and look it up.
			/// [JP] 生成側のスレッドで登録する。タスクが投入されてここを引くよりも前に済む。
			thread2Worker_.emplace(workers_[id].thread_.get_id(), &workers_[id]);
		}
	}

	/**
	* [EN]
	* Runs cache, then keeps popping and running nodes from worker's own
	* queue until it is empty; cache is left null.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* cache を実行し、その後 worker 自身のキューが空になるまでノードを
	* 取り出して実行し続ける。終わったとき cache は null になっている。
	*/
	void JobExecutor::ExploitTask(JobWorker& worker, JobNode*& cache)
	{
		/// [EN] The own queue is popped from the back (LIFO), which keeps recently pushed, cache-warm nodes on this thread.
		/// [JP] 自分のキューは後ろから（LIFO で）取る。直前に積んだ、キャッシュに乗ったノードをこのスレッドで続けて処理できる。
		while (cache)
		{
			Invoke(worker, cache);
			cache = worker.wsq_.pop();
		}
	}

	/**
	* [EN]
	* Tries to steal one node from another worker's queue or a buffer
	* into cache (it does not run it). Returns false only when worker
	* has been told to stop; true otherwise, whether or not anything was
	* stolen.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 他のワーカーのキューかバッファから、ノードを1つ cache へ盗み取る
	* （実行はしない）。false を返すのは worker に停止が指示されたとき
	* だけで、それ以外は盗めたかどうかに関係なく true を返す。
	*/
	Bool JobExecutor::ExploreTask(JobWorker& worker, JobNode*& cache)
	{
		/// [EN] With no topology running there is nothing to steal, so the caller goes straight to parking.
		/// [JP] 実行中のトポロジーが無ければ盗むものも無いので、呼び出し側はそのまま眠りに行く。
		if (numberTopologies_.load(std::memory_order_relaxed) == 0)
		{
			return true;
		}

		/// [EN] After about two full rounds of failed steals the thread starts yielding its time slice between attempts.
		/// [JP] 盗み取りにおよそ2周分失敗したら、試行の合間にタイムスライスを譲り始める。
		const Size MAX_VICTIM = NumberQueues();
		const Size MAX_STEALS = ((MAX_VICTIM + 1) << 1);

		Size numberSteals = 0;
		Size victim = worker.stickyVictim_;

		while (true)
		{
			/// [EN] Victim indices below the worker count are worker queues; the rest map onto the buffers.
			/// [JP] ワーカー数未満の番号はワーカーのキュー、それ以降はバッファに対応する。
			cache = (victim < workers_.size()) ? workers_[victim].wsq_.steal() : buffers_[victim - workers_.size()].queue_.steal();

			/// [EN] A victim that had work is likely to have more, so it becomes the first one tried next time.
			/// [JP] 仕事があった対象にはまだ残っている見込みが高いので、次回はそこから試す。
			if (cache)
			{
				worker.stickyVictim_ = victim;
				break;
			}

			/// [EN] Picks a random victim other than this worker's own index.
			/// [JP] このワーカー自身の番号を除いて、ランダムに対象を選ぶ。
			victim = worker.rdgen_() % (MAX_VICTIM - 1);
			if (victim >= worker.id_)
			{
				victim++;
			}

			/// [EN] Past the steal budget the thread yields, and after 150 more failures it gives up and lets the caller park.
			/// [JP] 上限を超えたらタイムスライスを譲り、さらに150回失敗したらあきらめて呼び出し側に眠らせる。
			if (++numberSteals > MAX_STEALS)
			{
				std::this_thread::yield();
				if (numberSteals > 150 + MAX_STEALS)
				{
					break;
				}
			}

			if (worker.done_.test(std::memory_order_relaxed))
			{
				return false;
			}
		}

		return true;
	}

	/**
	* [EN]
	* Pushes cache onto worker's own queue (or a buffer if that queue is
	* full) and wakes one parked worker to come and steal it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* cache を worker 自身のキュー（満杯であればバッファ）へ push し、
	* それを盗みに来るよう、眠っているワーカーを1つ起こす。
	*/
	void JobExecutor::Schedule(JobWorker& worker, JobNode*& cache)
	{
		/// [EN] The own queue is bounded; overflow goes to a shared buffer instead of growing it.
		/// [JP] 自分のキューは有界なので、あふれた分はキューを広げずに共有のバッファへ回す。
		if (worker.wsq_.try_push(cache) == false)
		{
			Spill(cache);
		}
		notifier_.notify_one();
	}

	/**
	* [EN]
	* Schedules cache from a thread that is not a worker, by putting it
	* in a buffer and waking one parked worker.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワーカーではないスレッドから cache をスケジューリングする。バッファ
	* へ入れ、眠っているワーカーを1つ起こす。
	*/
	void JobExecutor::Schedule(JobNode*& cache)
	{
		/// [EN] A non-worker has no queue of its own, so the buffer is the only place to put the node.
		/// [JP] ワーカーでないスレッドは自分のキューを持たないので、置き場所はバッファしかない。
		Spill(cache);
		notifier_.notify_one();
	}

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
	* 先行ノードを持たないものをスケジューリングする。その数を parent の
	* join カウンタへ足す。
	*/
	void JobExecutor::ScheduleGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent)
	{
		/// [EN] parent's counter tracks how many of its child nodes are still in flight, so the sources are added before any of them can finish.
		/// [JP] parent のカウンタは実行中の子ノードの数を表す。ソースのどれかが終わるより前に、その数を足しておく。
		Size numberSources = SetUpGraph(graph, topology, parent);
		parent->joinCounter_.fetch_add(numberSources, std::memory_order_relaxed);
		BulkSchedule(worker, graph.begin(), numberSources);
	}

	/**
	* [EN]
	* Puts node into one of the overflow buffer queues.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node を、あふれ用バッファキューのどれか1つへ入れる。
	*/
	void JobExecutor::Spill(JobNode* node)
	{
		/// [EN] The buffer is picked from the node's address, dropping the low bits that allocation alignment keeps constant.
		/// [JP] バッファはノードのアドレスから選ぶ。確保時の境界揃えで変化しない下位ビットは捨てる。
		auto buffer = (reinterpret_cast<uintptr_t>(node) >> 16) % buffers_.size();
		std::scoped_lock lock(buffers_[buffer].mutex_);
		buffers_[buffer].queue_.push(node);
	}

	/**
	* [EN]
	* Starts one run of topology's graph: prepares every node, sets the
	* topology's join counter to the number of source nodes and
	* schedules them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* topology のグラフの1回分の実行を始める。全ノードを整え、トポロジーの
	* join カウンタをソースノードの数にし、それらをスケジューリングする。
	*/
	void JobExecutor::SetUpTopology(JobWorker* worker, JobTopology* topology)
	{
		/// [EN] The topology is its own parent node, so its counter reaching zero means the whole graph is done.
		/// [JP] トポロジー自身が親ノードになるので、そのカウンタが 0 になれば、グラフ全体が終わったことになる。
		auto& graph = topology->taskflow_.graph_;
		Size numberSources = SetUpGraph(graph, topology, topology);
		topology->joinCounter_.store(numberSources, std::memory_order_relaxed);

		/// [EN] A run submitted from outside the pool has no worker queue to push to, so it goes through the buffers.
		/// [JP] プールの外から投入された実行には push 先のワーカーキューが無いので、バッファを通す。
		worker ? BulkSchedule(*worker, graph.begin(), numberSources) : BulkSchedule(graph.begin(), numberSources);
	}

	/**
	* [EN]
	* Resets every node of graph for a new run under topology/parent and
	* moves the source nodes (no predecessor) to the front of the graph.
	* Returns how many source nodes there are.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* graph の全ノードを、topology/parent のもとでの新しい実行に向けて
	* 初期化し、ソースノード（先行ノードなし）をグラフの先頭へ寄せる。
	* ソースノードの数を返す。
	*/
	Size JobExecutor::SetUpGraph(JobGraph& graph, JobTopology* topology, JobNodeBase* parent)
	{
		auto first = graph.begin();
		auto last = graph.end();

		/// [EN] send marks the end of the source nodes gathered at the front so far.
		/// [JP] send は、ここまでに先頭へ集めたソースノードの終わりを指す。
		auto send = first;

		for (;first != last;++first)
		{
			/// [EN] Everything left over from a previous run (flags, exception, counter) is cleared here.
			/// [JP] 前回の実行から残っているもの（フラグ、例外、カウンタ）はここで全て消す。
			auto node = *first;
			node->topology_ = topology;
			node->parent_ = parent;
			node->nstate_ = JobNodeState::NONE;
			node->estate_.store(JobExceptionState::NONE, std::memory_order_relaxed);
			node->SetUpJoinCounter();
			node->exceptionPtr_ = nullptr;

			/// [EN] Gathering the sources at the front lets them be scheduled as one contiguous range.
			/// [JP] ソースを先頭に集めることで、1つの連続した範囲としてまとめてスケジューリングできる。
			if (node->NumberPredecessors() == 0)
			{
				std::iter_swap(send++, first);
			}
		}
		return send - graph.begin();
	}

	/**
	* [EN]
	* Called when every node of topology's run has finished. Starts
	* another pass if the topology is not cancelled and its predicate is
	* still false; otherwise finishes the topology (callback, promise,
	* count) and moves on to the next run queued on the same taskflow,
	* or, when there is none, notifies the parent node that waited on it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* topology の実行で全ノードが終わったときに呼ばれる。キャンセルされて
	* おらず述語がまだ false なら、もう1周実行する。そうでなければ
	* トポロジーを終わらせ（コールバック、promise、数の更新）、同じタスク
	* フローに積まれた次の実行へ進む。次が無ければ、これを待っていた親
	* ノードへ知らせる。
	*/
	void JobExecutor::TearDownTopology(JobWorker& worker, JobTopology* topology, JobNode*& cache)
	{
		auto& flow = topology->taskflow_;

		/// [EN] The predicate is asked after each pass; false means "not done yet", so the same graph runs again.
		/// [JP] 述語は各周の後に聞く。false は「まだ終わりではない」なので、同じグラフをもう一度回す。
		if (!topology->Cancelled() && !topology->predicate_())
		{
			ScheduleGraph(worker, topology->taskflow_.graph_, topology, topology);
		}
		else
		{
			topology->onFinish_();

			/// [EN] Runs of one taskflow are queued and executed one after another, never at the same time.
			/// [JP] 1つのタスクフローの実行は列に積まれ、同時ではなく1つずつ順に実行される。
			if (std::unique_lock<std::mutex> lock(flow.mutex_);flow.topologies_.size() > 1)
			{
				/// [EN] The finished run is taken out of the queue, and the next one becomes the front.
				/// [JP] 終わった実行を列から取り出し、次の実行が先頭になる。
				auto fetchedTopology{ std::move(flow.topologies_.front()) };

				flow.topologies_.pop();
				topology = flow.topologies_.front().get();

				/// [EN] The promise is fulfilled outside the lock, since a waiter woken by it may submit to the same taskflow.
				/// [JP] promise はロックの外で満たす。それで起きた待機側が、同じタスクフローへ投入することがあるため。
				lock.unlock();

				fetchedTopology->CarryOutPromise();

				DecrementTopology();

				/// [EN] The next run reuses the same graph, so it starts right here on this worker.
				/// [JP] 次の実行は同じグラフを使うので、このワーカーでそのまま始める。
				ScheduleGraph(worker, topology->taskflow_.graph_, topology, topology);
			}
			else
			{
				auto fetchedTopology{ std::move(flow.topologies_.front()) };

				flow.topologies_.pop();

				lock.unlock();

				fetchedTopology->CarryOutPromise();

				DecrementTopology();

				/// [EN] A topology with a parent was run from inside another task; that task resumes once all its children are done.
				/// [JP] 親を持つトポロジーは、別のタスクの中から実行されたもの。その子が全て終わったら、そのタスクを再開する。
				if (auto parent = fetchedTopology->parent_;parent)
				{
					if (parent->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
					{
						UpdateCache(worker, cache, static_cast<JobNode*>(parent));
					}
				}
			}
		}
	}

	/**
	* [EN]
	* Accounts for a finished node on its parent: decrements the parent's
	* join counter and, if this was the last outstanding child, either
	* finishes the topology (when the parent is the topology itself) or
	* resumes the suspended parent node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 終わったノードを親の側に反映する。親の join カウンタを減らし、これが
	* 最後の子だった場合、親がトポロジー自身ならトポロジーを終わらせ、
	* そうでなければ中断している親ノードを再開する。
	*/
	void JobExecutor::TearDownNonasync(JobWorker& worker, JobNode* node, JobNode*& cache)
	{
		/// [EN] A top-level node's parent is the topology, whose counter reaching zero ends the pass.
		/// [JP] 最上位のノードの親はトポロジーで、そのカウンタが 0 になるとその周が終わる。
		if (auto parent = node->parent_;parent == node->topology_)
		{
			if (parent->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				TearDownTopology(worker, node->topology_, cache);
			}
		}
		else
		{
			/// [EN] The state is read before the decrement, because after it the parent may already be resumed and changing it.
			/// [JP] 状態は減らす前に読む。減らした後は、親が既に再開されて状態を変えている可能性がある。
			auto state = parent->nstate_;
			if (parent->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				/// [EN] Only a parent that suspended itself to wait (PREEMPTED) is put back to run; one that is coruning notices by itself.
				/// [JP] 待つために自分を中断した（PREEMPTED の）親だけを実行に戻す。Corun している親は自分で気づく。
				if (state & JobNodeState::PREEMPTED)
				{
					UpdateCache(worker, cache, static_cast<JobNode*>(parent));
				}
			}
		}
	}

	/**
	* [EN]
	* Tears node down without running it, as done for a node whose run
	* was cancelled.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node を実行せずに片付ける。実行がキャンセルされたノードに対して行う。
	*/
	void JobExecutor::TearDownInvoke(JobWorker& worker, JobNode* node, JobNode*& cache)
	{
		TearDownNonasync(worker, node, cache);
	}

	/**
	* [EN]
	* Increments the executor-wide outstanding topology count.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エグゼキュータ全体の未完了トポロジー数をインクリメントする。
	*/
	void JobExecutor::IncrementTopology()
	{
		numberTopologies_.fetch_add(1, std::memory_order_relaxed);
	}

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
	void JobExecutor::DecrementTopology()
	{
		/// [EN] Only the transition to zero is announced; WaitForAll ignores every other value.
		/// [JP] 知らせるのは 0 になったときだけ。WaitForAll はそれ以外の値を気にしない。
		if (numberTopologies_.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			numberTopologies_.notify_all();
		}
	}

	/**
	* [EN]
	* Runs node according to its NodeHandle alternative, then releases
	* its semaphores, makes ready the successors it unblocked and tears
	* it down. One newly ready successor is kept in a local cache and
	* run next by jumping back to the start, so a long chain of nodes
	* runs in a loop instead of growing the call stack.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node をその NodeHandle の選択肢に従って実行し、その後セマフォを解放し、
	* 待ちが解けた後続ノードを実行可能にし、node を片付ける。新たに実行
	* 可能になった後続のうち1つはローカルの cache に残し、先頭へ戻って
	* 次に実行する。長く連なるノードも、呼び出しスタックを伸ばさずに
	* ループで処理できる。
	*/
	void JobExecutor::Invoke(JobWorker& worker, JobNode* node)
	{
		/// [EN] Continues with the cached successor, if any, by jumping back to the start instead of recursing.
		/// [JP] cache に後続があれば、再帰せずに先頭へ戻ってそれを続けて実行する。
#define SC_INVOKE_CONTINUATION() \
		if (cache) \
		{ \
			node = cache; \
			goto beginInvoke; \
		}

	beginInvoke:

		/// [EN] The successor chosen to run next on this thread; any others are pushed to the queue.
		/// [JP] このスレッドで次に実行する後続。それ以外はキューへ積まれる。
		JobNode* cache = nullptr;

		/// [EN] A resumed node already passed the cancel and semaphore checks on its first entry.
		/// [JP] 再開されたノードは、最初の実行時にキャンセルとセマフォの確認を済ませている。
		if (node->nstate_ & JobNodeState::PREEMPTED)
		{
			goto invokeTask;
		}

		/// [EN] A cancelled or failed run skips the work, but the node is still torn down so its parent's count stays right.
		/// [JP] キャンセルや失敗した実行では処理を飛ばす。ただし親の数が狂わないよう、片付けは行う。
		if (node->ParentCancelled())
		{
			TearDownInvoke(worker, node, cache);
			SC_INVOKE_CONTINUATION();
			return;
		}

		/// [EN] A node that cannot take all its semaphores is parked on one of them; any tasks released by the rollback are rescheduled.
		/// [JP] セマフォを全て取れなかったノードは、そのどれかで待機させる。巻き戻しで解放されたタスクはスケジュールし直す。
		if (node->semaphores_ && !node->semaphores_->acquire_.empty())
		{
			HybridArray<JobNode*> waiters;
			if (!node->AcquireAll(waiters))
			{
				BulkSchedule(worker, waiters.begin(), waiters.size());
				return;
			}
		}

	invokeTask:

		/// [EN] Successor indices chosen by a condition node.
		/// [JP] 条件ノードが選んだ後続のインデックス。
		HybridArray<Int> conds;

		/// [EN] Handles that start child work return true while those children run; the node is resumed later and finishes then.
		/// [JP] 子の処理を始めるハンドルは、子が走っている間 true を返す。ノードは後で再開され、そのときに終わる。
		switch (node->handle_.index())
		{
		case JobNode::STATIC:
		{
			InvokeStaticTask(worker, node);
		}
		break;
		case JobNode::PREEMPTIVE_RUNTIME:
		{
			if (InvokePreemptiveRuntimeTask(worker, node))
			{
				return;
			}
		}
		break;
		case JobNode::NONPREEMPTIVE_RUNTIME:
		{
			InvokeNonpreemptiveRuntimeTask(worker, node);
		}
		break;
		case JobNode::SUBFLOW:
		{
			if (InvokeSubflowTask(worker, node))
			{
				return;
			}
		}
		break;
		case JobNode::SINGLE_CONDITION:
		{
			InvokeSingleConditionTask(worker, node, conds);
		}
		break;
		case JobNode::MULTI_CONDITION:
		{
			InvokeMultiConditionTask(worker, node, conds);
		}
		break;
		case JobNode::OWNED_MODULE:
		{
			if (InvokeOwnedModuleTask(worker, node))
			{
				return;
			}
		}
		break;
		case JobNode::ADOPTED_MODULE:
		{
			if (InvokeAdoptedModuleTask(worker, node))
			{
				return;
			}
		}
		break;
		default:
			break;
		}

		/// [EN] Tasks waiting on the released semaphores are rescheduled to try again.
		/// [JP] 解放したセマフォを待っていたタスクを、もう一度試させるためにスケジュールし直す。
		if (node->semaphores_ && !node->semaphores_->release_.empty())
		{
			HybridArray<JobNode*> waiters;
			node->ReleaseAll(waiters);
			BulkSchedule(worker, waiters.begin(), waiters.size());
		}

		/// [EN] The counter is restored to the strong-dependency count, ready for the next pass of the graph.
		/// [JP] カウンタを強い依存の数に戻し、グラフの次の周に備える。
		node->joinCounter_.fetch_add(node->nstate_ & JobNodeState::STRONG_DEPENDENCIES_MASK, std::memory_order_relaxed);

		switch (node->handle_.index())
		{
		case JobNode::SINGLE_CONDITION:
			[[fallthrough]];
		case JobNode::MULTI_CONDITION:
		{
			/// [EN] A condition runs the chosen successors directly, whatever their counters say; out-of-range indices are ignored.
			/// [JP] 条件ノードは、カウンタに関係なく選んだ後続を直接実行する。範囲外のインデックスは無視する。
			for (Int cond : conds)
			{
				if (cond >= 0 && static_cast<Size>(cond) < node->numberSuccessors_)
				{
					auto s = node->edges_[cond];
					s->joinCounter_.store(0, std::memory_order_relaxed);
					node->parent_->joinCounter_.fetch_add(1, std::memory_order_relaxed);
					UpdateCache(worker, cache, s);
				}
			}
		}
		break;
		default:
		{
			/// [EN] A successor becomes ready when this node was the last of its predecessors to finish.
			/// [JP] 後続は、このノードがその先行ノードのうち最後に終わったものだったときに実行可能になる。
			for (Size index = 0;index < node->numberSuccessors_;++index)
			{
				if (auto s = node->edges_[index];s->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					/// [EN] The parent counts the successor as in flight before it can possibly finish.
					/// [JP] 後続が終わりうるより前に、親の側でそれを実行中として数えておく。
					node->parent_->joinCounter_.fetch_add(1, std::memory_order_relaxed);
					UpdateCache(worker, cache, s);
				}
			}
		}
		break;
		}

		TearDownNonasync(worker, node, cache);
		SC_INVOKE_CONTINUATION();
	}

	/**
	* [EN]
	* Invokes a Static task's callable.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Static タスクの呼び出し可能オブジェクトを実行する。
	*/
	void JobExecutor::InvokeStaticTask(JobWorker& worker, JobNode* node)
	{
#if !SC_DISABLE_EXCEPTION_HANDLING
		try
		{
#endif
			std::get_if<JobNode::Static>(&node->handle_)->work_();
#if !SC_DISABLE_EXCEPTION_HANDLING
		}
		catch (...)
		{
			/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
			/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
			ProcessException(worker, node);
		}
#endif
	}

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
	void JobExecutor::InvokeNonpreemptiveRuntimeTask(JobWorker& worker, JobNode* node)
	{
		/// [EN] The runtime only lives for this call; anything it starts must finish before the callable returns.
		/// [JP] ランタイムはこの呼び出しの間だけ存在する。そこで始めたものは、呼び出しが戻る前に終わっている必要がある。
		JobNonpreemptiveRuntime nonruntime(*this, worker);
#if !SC_DISABLE_EXCEPTION_HANDLING
		try
		{
#endif
			std::get_if<JobNode::NonpreemptiveRuntime>(&node->handle_)->work_(nonruntime);
#if !SC_DISABLE_EXCEPTION_HANDLING
		}
		catch (...)
		{
			/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
			/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
			ProcessException(worker, node);
		}
#endif
	}

	/**
	* [EN]
	* Invokes a SingleCondition task's callable and replaces conds with
	* the single successor index it returns.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SingleCondition タスクの呼び出し可能オブジェクトを実行し、conds を
	* それが返した単一の後続インデックスで置き換える。
	*/
	void JobExecutor::InvokeSingleConditionTask(JobWorker& worker, JobNode* node, HybridArray<Int>& conds)
	{
		auto& work = std::get_if<JobNode::SingleCondition>(&node->handle_)->work_;
#if !SC_DISABLE_EXCEPTION_HANDLING
		try
		{
#endif
			conds = { work() };
#if !SC_DISABLE_EXCEPTION_HANDLING
		}
		catch (...)
		{
			/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
			/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
			ProcessException(worker, node);
		}
#endif
	}

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
	void JobExecutor::InvokeMultiConditionTask(JobWorker& worker, JobNode* node, HybridArray<Int>& conds)
	{
#if !SC_DISABLE_EXCEPTION_HANDLING
		try
		{
#endif
			conds = std::get_if<JobNode::MultiCondition>(&node->handle_)->work_();
#if !SC_DISABLE_EXCEPTION_HANDLING
		}
		catch (...)
		{
			/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
			/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
			ProcessException(worker, node);
		}
#endif
	}

	/**
	* [EN]
	* On first entry, creates the JobSubflow builder for the node and,
	* unless the subflow was already joined and the subgraph is not
	* empty, schedules the subgraph and suspends the node (returns
	* true). On the resuming call, after the subgraph has finished,
	* clears the subgraph unless it was marked to be retained, and
	* returns false so the node finishes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 初回はノード用の JobSubflow ビルダーを作り、まだ合流しておらず
	* サブグラフが空でなければ、サブグラフをスケジューリングしてノードを
	* 中断する（true を返す）。サブグラフが終わった後の再開時の呼び出し
	* では、保持するよう指定されていない限りサブグラフを消し、ノードが
	* 終わるよう false を返す。
	*/
	Bool JobExecutor::InvokeSubflowTask(JobWorker& worker, JobNode* node)
	{
		auto& handle = *std::get_if<JobNode::Subflow>(&node->handle_);
		auto& graph = handle.subgraph_;

		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			/// [EN] Constructing the builder resets the subgraph and the joined/retain flags for this run.
			/// [JP] ビルダーを構築すると、この実行に向けてサブグラフと合流済み/保持のフラグが初期化される。
			JobSubflow subflow(*this, worker, node, graph);

			/// [EN] The user's callable fills the subgraph through the builder (and may join it itself).
			/// [JP] 利用者の処理がビルダーを通じてサブグラフを埋める（自分で合流することもある）。
#if !SC_DISABLE_EXCEPTION_HANDLING
			try
			{
#endif
				handle.work_(subflow);
#if !SC_DISABLE_EXCEPTION_HANDLING
			}
			catch (...)
			{
				/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
				/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
				ProcessException(worker, node);
			}
#endif

			/// [EN] Once scheduled, the node waits as PREEMPTED and is put back to run by the last child to finish.
			/// [JP] スケジュールした後、ノードは PREEMPTED で待ち、最後に終わった子によって実行に戻される。
			if (subflow.Joinable() && !graph.empty())
			{
				node->nstate_ |= JobNodeState::PREEMPTED;

				ScheduleGraph(worker, graph, node->topology_, node);
				return true;
			}
		}
		else
		{
			node->nstate_ &= ~JobNodeState::PREEMPTED;
		}

		/// [EN] The subgraph is rebuilt on every run unless the user asked to keep it.
		/// [JP] 利用者が残すよう求めていなければ、サブグラフは実行のたびに作り直す。
		if ((node->nstate_ & JobNodeState::RETAIN_SUBFLOW) == 0)
		{
			graph.clear();
		}

		return false;
	}

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
	Bool JobExecutor::InvokeOwnedModuleTask(JobWorker& worker, JobNode* node)
	{
		return InvokeModuleTaskImplementation(worker, node, std::get_if<JobNode::OwnedModule>(&node->handle_)->graph_);
	}

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
	Bool JobExecutor::InvokeAdoptedModuleTask(JobWorker& worker, JobNode* node)
	{
		return InvokeModuleTaskImplementation(worker, node, std::get_if<JobNode::AdoptedModule>(&node->handle_)->graph_);
	}

	/**
	* [EN]
	* Shared implementation for OwnedModule/AdoptedModule: on first
	* entry schedules graph as node's children and suspends the node
	* (returns true); on the resuming call, clears the preempted flag
	* and returns false so the node finishes. An empty graph finishes at
	* once.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OwnedModule/AdoptedModule の共通実装。初回は graph を node の子として
	* スケジューリングしてノードを中断する（true を返す）。再開時は
	* プリエンプトフラグを消し、ノードが終わるよう false を返す。空の
	* グラフはすぐに終わる。
	*/
	Bool JobExecutor::InvokeModuleTaskImplementation(JobWorker& worker, JobNode* node, JobGraph& graph)
	{
		if (graph.empty())
		{
			return false;
		}

		/// [EN] The node waits as PREEMPTED until the last node of the module graph puts it back to run.
		/// [JP] モジュールのグラフの最後のノードが実行に戻すまで、ノードは PREEMPTED で待つ。
		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			node->nstate_ |= JobNodeState::PREEMPTED;
			ScheduleGraph(worker, graph, node->topology_, node);
			return true;
		}

		node->nstate_ &= ~JobNodeState::PREEMPTED;

		return false;
	}

	/**
	* [EN]
	* Invokes a PreemptiveRuntime task's callable by delegating to
	* InvokeRuntimeTaskImplementation. Returns whether the node was
	* suspended to wait for work it started.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* InvokeRuntimeTaskImplementation へ委譲して PreemptiveRuntime
	* タスクの呼び出し可能オブジェクトを実行する。始めた処理を待つために
	* ノードを中断したかどうかを返す。
	*/
	Bool JobExecutor::InvokePreemptiveRuntimeTask(JobWorker& worker, JobNode* node)
	{
		return InvokeRuntimeTaskImplementation(worker, node, std::get_if<JobNode::PreemptiveRuntime>(&node->handle_)->work_);
	}

	/**
	* [EN]
	* Calls function(runtime) on first entry. If the tasks it spawned
	* through the runtime are still running when it returns, the node is
	* left suspended (returns true) and is resumed by the last of them;
	* the resuming call only clears the flags and returns false.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 初回は function(runtime) を呼ぶ。戻った時点でランタイム経由で生成した
	* タスクがまだ走っていれば、ノードを中断したままにし（true を返す）、
	* それらの最後の1つによって再開される。再開時の呼び出しはフラグを
	* 消して false を返すだけ。
	*/
	Bool JobExecutor::InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&)>& function)
	{
		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			JobPreemptiveRuntime runtime(*this, worker, node);

			/// [EN] IMPLICITLY_ANCHORED makes this node the catcher of exceptions thrown by the tasks it spawns.
			/// [JP] IMPLICITLY_ANCHORED により、このノードが、自分の生成したタスクの投げた例外の受け手になる。
			node->nstate_ |= (JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);

			/// [EN] The extra count keeps a child that finishes during the callable from resuming the node before the callable returns.
			/// [JP] 余分に1つ数えておくことで、処理の途中で終わった子が、処理が戻る前にノードを再開させることを防ぐ。
			node->joinCounter_.fetch_add(1, std::memory_order_release);

#if !SC_DISABLE_EXCEPTION_HANDLING
			try
			{
#endif
				function(runtime);
#if !SC_DISABLE_EXCEPTION_HANDLING
			}
			catch (...)
			{
				/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
				/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
				ProcessException(worker, node);
			}
#endif

			/// [EN] Releasing the extra count tells whether spawned tasks are still running; if so, the node stays suspended.
			/// [JP] 余分なカウントを返すと、生成したタスクがまだ走っているかが分かる。走っていれば、ノードは中断したままになる。
			if (node->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				node->nstate_ &= ~(JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);
			}
			else
			{
				return true;
			}
		}
		else
		{
			node->nstate_ &= ~(JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);
		}

		return false;
	}

	/**
	* [EN]
	* Variant for callables that also receive a "resumed" flag: calls
	* function(runtime, false) on first entry and, once every task it
	* spawned has finished (right away or on a later resume), calls
	* function(runtime, true) before the node finishes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 「再開か」のフラグも受け取る処理向けの版。初回は function(runtime, false)
	* を呼び、生成したタスクが全て終わった時点で（その場で、あるいは後の
	* 再開時に）ノードが終わる前に function(runtime, true) を呼ぶ。
	*/
	Bool JobExecutor::InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&, Bool)>& function)
	{
		JobPreemptiveRuntime runtime(*this, worker, node);

		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			/// [EN] The extra count keeps a child that finishes during the callable from resuming the node before the callable returns.
			/// [JP] 余分に1つ数えておくことで、処理の途中で終わった子が、処理が戻る前にノードを再開させることを防ぐ。
			node->nstate_ |= (JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);
			node->joinCounter_.fetch_add(1, std::memory_order_release);

#if !SC_DISABLE_EXCEPTION_HANDLING
			try
			{
#endif
				function(runtime, false);
#if !SC_DISABLE_EXCEPTION_HANDLING
			}
			catch (...)
			{
				/// [EN] The exception is recorded on the node's anchor instead of escaping the worker loop.
				/// [JP] 例外はワーカーのループから漏らさず、ノードのアンカーに記録する。
				ProcessException(worker, node);
			}
#endif

			if (node->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				node->nstate_ &= ~(JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);
			}
			else
			{
				return true;
			}
		}
		else
		{
			node->nstate_ &= ~(JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);
		}

		/// [EN] Reached once every spawned task is done, so the callable can gather their results.
		/// [JP] 生成したタスクが全て終わってからここへ来るので、処理はそれらの結果を集められる。
		function(runtime, true);

		return false;
	}

	/**
	* [EN]
	* Records the exception currently in flight: flags node and every
	* ancestor up to the nearest explicit anchor with EXCEPTION (which
	* cancels the rest of their work), and stores the exception on that
	* explicit anchor (a blocked corun/join or the topology), or else on
	* the nearest implicit anchor (a runtime task), or else on node itself.
	* Only the first exception reaching an anchor is kept.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 今投げられている例外を記録する。node から最も近い明示アンカーの手前
	* までの全ての段に EXCEPTION を立て（残りの処理はキャンセル扱いになる）、
	* 例外はその明示アンカー（待っている corun/join、あるいはトポロジー）に、
	* 無ければ最も近い暗黙アンカー（ランタイムタスク）に、それも無ければ
	* node 自身に格納する。アンカーに届いた最初の例外だけを残す。
	*/
	void JobExecutor::ProcessException(JobWorker& worker, JobNode* node)
	{
		JobNodeBase* explicitAnchor = node;
		JobNodeBase* implicitAnchor = nullptr;

		/// [EN] Walks up the parent chain until an explicit anchor, flagging every level and remembering the innermost implicit anchor.
		/// [JP] 明示アンカーに着くまで親を辿り、全ての段に印を付けながら、最も内側の暗黙アンカーを覚えておく。
		while (explicitAnchor && (explicitAnchor->estate_.load(std::memory_order_relaxed) & JobExceptionState::EXPLICITLY_ANCHORED) == 0)
		{
			explicitAnchor->estate_.fetch_or(JobExceptionState::EXCEPTION, std::memory_order_relaxed);
			if (implicitAnchor == nullptr && (explicitAnchor->nstate_ & JobNodeState::IMPLICITLY_ANCHORED))
			{
				implicitAnchor = explicitAnchor;
			}
			explicitAnchor = explicitAnchor->parent_;
		}

		constexpr static auto flag = JobExceptionState::EXCEPTION | JobExceptionState::CAUGHT;

		/// [EN] A blocked caller takes priority; CAUGHT is set atomically so only the first of several failures is stored.
		/// [JP] 待っている呼び出し側が優先。CAUGHT を不可分に立てるので、複数の失敗のうち最初の1つだけが格納される。
		if (explicitAnchor)
		{
			if ((explicitAnchor->estate_.fetch_or(flag, std::memory_order_relaxed) & JobExceptionState::CAUGHT) == 0)
			{
				explicitAnchor->exceptionPtr_ = std::current_exception();
				return;
			}
		}
		/// [EN] With no explicit anchor, the innermost runtime task catches it.
		/// [JP] 明示アンカーが無ければ、最も内側のランタイムタスクが受け取る。
		else if (implicitAnchor)
		{
			if ((implicitAnchor->estate_.fetch_or(flag, std::memory_order_relaxed) & JobExceptionState::CAUGHT) == 0)
			{
				implicitAnchor->exceptionPtr_ = std::current_exception();
				return;
			}
		}

		/// [EN] Nothing is waiting for it, so the exception is kept on the node itself.
		/// [JP] 待っている者がいないので、例外はノード自身に残す。
		node->exceptionPtr_ = std::current_exception();
	}

	/**
	* [EN]
	* Makes node the next one to run on this thread, first scheduling
	* whatever was previously in cache so other workers can take it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node を、このスレッドで次に実行するものにする。その前に、それまで
	* cache にあったものをスケジューリングし、他のワーカーが取れるようにする。
	*/
	void JobExecutor::UpdateCache(JobWorker& worker, JobNode*& cache, JobNode* node)
	{
		/// [EN] Only the latest ready node stays local; earlier ones are published for stealing.
		/// [JP] 手元に残すのは最後に実行可能になったものだけ。それより前のものは盗み取れるよう公開する。
		if (cache)
		{
			Schedule(worker, cache);
		}
		cache = node;
	}

	/**
	* [EN]
	* Schedules graph under topology/parent on worker and keeps worker
	* running tasks (its own or stolen) until every node of graph has
	* finished, so the calling task blocks without idling its thread.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* worker 上で graph を topology/parent のもとでスケジューリングし、
	* graph の全ノードが終わるまで worker にタスク（自分のものでも盗んだ
	* ものでも）を実行させ続ける。呼び出し元のタスクは、スレッドを遊ばせ
	* ずに待てる。
	*/
	void JobExecutor::CorunGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent)
	{
		if (graph.empty())
		{
			return;
		}

		{
			/// [EN] parent is the blocking point, so exceptions from the graph are stored on it while this scope lasts.
			/// [JP] parent が待つ地点なので、このスコープの間、graph からの例外は parent に格納される。
			JobExplicitAnchorGuard anchor(parent);

			/// [EN] parent's counter falls back to zero once every node of graph has finished.
			/// [JP] graph の全ノードが終わると、parent のカウンタは 0 に戻る。
			ScheduleGraph(worker, graph, topology, parent);
			CorunUntil(worker, [parent]()->Bool {return parent->joinCounter_.load(std::memory_order_acquire) == 0;});
		}

		/// [EN] An exception thrown inside the graph surfaces here, in the caller that waited for it.
		/// [JP] graph の中で投げられた例外は、ここで、それを待っていた呼び出し側へ出てくる。
		parent->RethrowException();
	}

	/**
	* [EN]
	* Finds the next node for worker: steals one if possible, and
	* otherwise parks the thread on the notifier until new work is
	* published. Returns true with a node in cache, or false once worker
	* has been told to stop.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* worker の次のノードを探す。盗めるなら盗み、無ければ新しい仕事が
	* 公開されるまで notifier でスレッドを眠らせる。cache にノードを入れて
	* true を返すか、worker に停止が指示されていれば false を返す。
	*/
	Bool JobExecutor::WaitForTask(JobWorker& worker, JobNode*& cache)
	{
	exploreTask:

		if (ExploreTask(worker, cache) == false)
		{
			return false;
		}

		if (cache)
		{
			return true;
		}

		/// [EN] Registering as a waiter before the final checks means work published from here on wakes this thread.
		/// [JP] 最後の確認より前に待機者として登録するので、ここから後に公開された仕事でこのスレッドは起こされる。
		notifier_.prepare_wait(worker.id_);

		/// [EN] With no topology running, the thread sleeps until the next submission or shutdown.
		/// [JP] 実行中のトポロジーが無ければ、次の投入か終了までスレッドを眠らせる。
		if (numberTopologies_.load(std::memory_order_relaxed) == 0)
		{
			if (worker.done_.test(std::memory_order_relaxed))
			{
				notifier_.cancel_wait(worker.id_);
				return false;
			}
			notifier_.commit_wait(worker.id_);
			goto exploreTask;
		}

		/// [EN] Work that appeared in a buffer during the steal attempts cancels the sleep and is stolen first.
		/// [JP] 盗み取りの間にバッファへ仕事が現れていれば、眠るのをやめて、そこから先に盗む。
		for (Size buffer = 0;buffer < buffers_.size();++buffer)
		{
			if (!buffers_[buffer].queue_.empty())
			{
				notifier_.cancel_wait(worker.id_);
				worker.stickyVictim_ = buffer + workers_.size();
				goto exploreTask;
			}
		}

		/// [EN] The same check over every other worker's queue, skipping this worker's own index.
		/// [JP] 同じ確認を、このワーカー自身を除く全ワーカーのキューに対して行う。
		for (Size workerIndex = 0;workerIndex < workers_.size() - 1;++workerIndex)
		{
			if (Size victim = workerIndex + (workerIndex >= worker.id_);!workers_[victim].wsq_.empty())
			{
				notifier_.cancel_wait(worker.id_);
				worker.stickyVictim_ = victim;
				goto exploreTask;
			}
		}

		if (worker.done_.test(std::memory_order_relaxed))
		{
			notifier_.cancel_wait(worker.id_);
			return false;
		}

		/// [EN] Nothing anywhere: sleep until a notify, then start looking again.
		/// [JP] どこにも無いので、通知が来るまで眠り、起きたらまた探し始める。
		notifier_.commit_wait(worker.id_);
		goto exploreTask;
	}
}
