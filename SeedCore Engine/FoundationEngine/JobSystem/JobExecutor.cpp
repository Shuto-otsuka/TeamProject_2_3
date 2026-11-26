#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>
#include <FoundationEngine/JobSystem/JobTopology.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	JobExecutor::JobExecutor(Size n, ResourceRef<JobWorkerInterface> worker) :workers_(n), buffers_(std::bit_width(n)), notifier_(n)
	{
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
			Shutdown();
			std::rethrow_exception(std::current_exception());
		}
#endif
	}

	JobExecutor::~JobExecutor()
	{
		Shutdown();
	}

	JobFuture<void> JobExecutor::Run(JobTaskflow& taskflow)
	{
		return RunNumber(taskflow, 1, []() {});
	}

	JobFuture<void> JobExecutor::Run(JobTaskflow&& taskflow)
	{
		return RunNumber(std::move(taskflow), 1, []() {});
	}

	JobFuture<void> JobExecutor::RunNumber(JobTaskflow& taskflow, Size n)
	{
		return RunNumber(taskflow, n, []() {});
	}

	JobFuture<void> JobExecutor::RunNumber(JobTaskflow&& taskflow, Size n)
	{
		return RunNumber(std::move(taskflow), n, []() {});
	}

	void JobExecutor::WaitForAll()
	{
		Size n = numberTopologies_.load(std::memory_order_acquire);
		while (n != 0)
		{
			numberTopologies_.wait(n, std::memory_order_acquire);
			n = numberTopologies_.load(std::memory_order_acquire);
		}
	}

	Size JobExecutor::NumberWorkers()const noexcept
	{
		return workers_.size();
	}

	Size JobExecutor::NumberWaiters()const noexcept
	{
		return notifier_.count();
	}

	Size JobExecutor::NumberQueues()const noexcept
	{
		return workers_.size() + buffers_.size();
	}

	Size JobExecutor::NumberTopologies()const noexcept
	{
		return numberTopologies_.load(std::memory_order_relaxed);
	}

	JobWorker* JobExecutor::ThisWorker()
	{
		auto iterator = thread2Worker_.find(std::this_thread::get_id());
		return iterator == thread2Worker_.end() ? nullptr : iterator->second;
	}

	Int JobExecutor::ThisWorkerID()const
	{
		auto index = thread2Worker_.find(std::this_thread::get_id());
		return index == thread2Worker_.end() ? -1 : static_cast<Int>(index->second->id_);
	}

	void JobExecutor::Shutdown()
	{
		WaitForAll();

		for (Size index = 0;index < workers_.size();++index)
		{
			workers_[index].done_.test_and_set(std::memory_order_relaxed);
		}

		notifier_.notify_all();

		for (JobWorker& worker : workers_)
		{
			if (worker.thread_.joinable())
			{
				worker.thread_.join();
			}
		}
	}

	void JobExecutor::Spawn(Size n, ResourceRef<JobWorkerInterface> workerInterface)
	{
		for (Size id = 0;id < n;++id)
		{
			workers_[id].thread_ = std::thread([&, id, workerInterface]()
				{
					auto& worker = workers_[id];

					worker.id_ = id;
					worker.stickyVictim_ = id;

					worker.rdgen_.Seed(static_cast<Uint32>(std::hash<std::thread::id>()(std::this_thread::get_id())));

					if (workerInterface)
					{
						workerInterface->SchedulerPrologue(worker);
					}

					JobNode* t = nullptr;

					std::exception_ptr ptr = nullptr;

#if !SC_DISABLE_EXCEPTION_HANDLING
					try
					{
#endif
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

			thread2Worker_.emplace(workers_[id].thread_.get_id(), &workers_[id]);
		}
	}

	void JobExecutor::ExploitTask(JobWorker& worker, JobNode*& cache)
	{
		while (cache)
		{
			Invoke(worker, cache);
			cache = worker.wsq_.pop();
		}
	}

	Bool JobExecutor::ExploreTask(JobWorker& worker, JobNode*& cache)
	{
		if (numberTopologies_.load(std::memory_order_relaxed) == 0)
		{
			return true;
		}

		const Size MAX_VICTIM = NumberQueues();
		const Size MAX_STEALS = ((MAX_VICTIM + 1) << 1);

		Size numberSteals = 0;
		Size victim = worker.stickyVictim_;

		while (true)
		{
			cache = (victim < workers_.size()) ? workers_[victim].wsq_.steal() : buffers_[victim - workers_.size()].queue_.steal();

			if (cache)
			{
				worker.stickyVictim_ = victim;
				break;
			}

			victim = worker.rdgen_() % (MAX_VICTIM - 1);
			if (victim >= worker.id_)
			{
				victim++;
			}

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

	void JobExecutor::Schedule(JobWorker& worker, JobNode*& cache)
	{
		if (worker.wsq_.try_push(cache) == false)
		{
			Spill(cache);
		}
		notifier_.notify_one();
	}

	void JobExecutor::Schedule(JobNode*& cache)
	{
		Spill(cache);
		notifier_.notify_one();
	}

	void JobExecutor::ScheduleGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent)
	{
		Size numberSources = SetUpGraph(graph, topology, parent);
		parent->joinCounter_.fetch_add(numberSources, std::memory_order_relaxed);
		BulkSchedule(worker, graph.begin(), numberSources);
	}

	void JobExecutor::Spill(JobNode* node)
	{
		auto buffer = (reinterpret_cast<uintptr_t>(node) >> 16) % buffers_.size();
		std::scoped_lock lock(buffers_[buffer].mutex_);
		buffers_[buffer].queue_.push(node);
	}

	void JobExecutor::SetUpTopology(JobWorker* worker, JobTopology* topology)
	{
		auto& graph = topology->taskflow_.graph_;
		Size numberSources = SetUpGraph(graph, topology, topology);
		topology->joinCounter_.store(numberSources, std::memory_order_relaxed);

		worker ? BulkSchedule(*worker, graph.begin(), numberSources) : BulkSchedule(graph.begin(), numberSources);
	}

	Size JobExecutor::SetUpGraph(JobGraph& graph, JobTopology* topology, JobNodeBase* parent)
	{
		auto first = graph.begin();
		auto last = graph.end();

		auto send = first;

		for (;first != last;++first)
		{
			auto node = *first;
			node->topology_ = topology;
			node->parent_ = parent;
			node->nstate_ = JobNodeState::NONE;
			node->estate_.store(JobExceptionState::NONE, std::memory_order_relaxed);
			node->SetUpJoinCounter();
			node->exceptionPtr_ = nullptr;

			if (node->NumberPredecessors() == 0)
			{
				std::iter_swap(send++, first);
			}
		}
		return send - graph.begin();
	}

	void JobExecutor::TearDownTopology(JobWorker& worker, JobTopology* topology, JobNode*& cache)
	{
		auto& flow = topology->taskflow_;

		if (!topology->Cancelled() && !topology->predicate_())
		{
			ScheduleGraph(worker, topology->taskflow_.graph_, topology, topology);
		}
		else
		{
			topology->onFinish_();

			if (std::unique_lock<std::mutex> lock(flow.mutex_);flow.topologies_.size() > 1)
			{
				auto fetchedTopology{ std::move(flow.topologies_.front()) };

				flow.topologies_.pop();
				topology = flow.topologies_.front().get();

				lock.unlock();

				fetchedTopology->CarryOutPromise();

				DecrementTopology();

				ScheduleGraph(worker, topology->taskflow_.graph_, topology, topology);
			}
			else
			{
				auto fetchedTopology{ std::move(flow.topologies_.front()) };

				flow.topologies_.pop();

				lock.unlock();

				fetchedTopology->CarryOutPromise();

				DecrementTopology();

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

	void JobExecutor::TearDownNonasync(JobWorker& worker, JobNode* node, JobNode*& cache)
	{
		if (auto parent = node->parent_;parent == node->topology_)
		{
			if (parent->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				TearDownTopology(worker, node->topology_, cache);
			}
		}
		else
		{
			auto state = parent->nstate_;
			if (parent->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				if (state & JobNodeState::PREEMPTED)
				{
					UpdateCache(worker, cache, static_cast<JobNode*>(parent));
				}
			}
		}
	}

	void JobExecutor::TearDownInvoke(JobWorker& worker, JobNode* node, JobNode*& cache)
	{
		TearDownNonasync(worker, node, cache);
	}

	void JobExecutor::IncrementTopology()
	{
		numberTopologies_.fetch_add(1, std::memory_order_relaxed);
	}

	void JobExecutor::DecrementTopology()
	{
		if (numberTopologies_.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			numberTopologies_.notify_all();
		}
	}

	void JobExecutor::Invoke(JobWorker& worker, JobNode* node)
	{
#define SC_INVOKE_CONTINUATION() \
		if (cache) \
		{ \
			node = cache; \
			goto beginInvoke; \
		}

	beginInvoke:

		JobNode* cache = nullptr;

		if (node->nstate_ & JobNodeState::PREEMPTED)
		{
			goto invokeTask;
		}

		if (node->ParentCancelled())
		{
			TearDownInvoke(worker, node, cache);
			SC_INVOKE_CONTINUATION();
			return;
		}

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

		HybridArray<Int> conds;

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

		if (node->semaphores_ && !node->semaphores_->release_.empty())
		{
			HybridArray<JobNode*> waiters;
			node->ReleaseAll(waiters);
			BulkSchedule(worker, waiters.begin(), waiters.size());
		}

		node->joinCounter_.fetch_add(node->nstate_ & JobNodeState::STRONG_DEPENDENCIES_MASK, std::memory_order_relaxed);

		switch (node->handle_.index())
		{
		case JobNode::SINGLE_CONDITION:
			FALLTHROUGH;
		case JobNode::MULTI_CONDITION:
		{
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
			for (Size index = 0;index < node->numberSuccessors_;++index)
			{
				if (auto s = node->edges_[index];s->joinCounter_.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
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

	void JobExecutor::InvokeStaticTask(JobWorker& worker, JobNode* node)
	{
		Bool named = !node->name_.view().empty();
		if (named)
		{
			Benchmark::Begin(node->name_);
		}
		else
		{
			Benchmark::Begin("Job");
		}

#if !SC_DISABLE_EXCEPTION_HANDLING
		try
		{
#endif
			std::get_if<JobNode::Static>(&node->handle_)->work_();
#if !SC_DISABLE_EXCEPTION_HANDLING
		}
		catch (...)
		{
			ProcessException(worker, node);
		}
#endif

		if (named)
		{
			Benchmark::End(node->name_);
		}
		else
		{
			Benchmark::End("Job");
		}
	}

	void JobExecutor::InvokeNonpreemptiveRuntimeTask(JobWorker& worker, JobNode* node)
	{
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
			ProcessException(worker, node);
		}
#endif
	}

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
			ProcessException(worker, node);
		}
#endif
	}

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
			ProcessException(worker, node);
		}
#endif
	}

	Bool JobExecutor::InvokeSubflowTask(JobWorker& worker, JobNode* node)
	{
		auto& handle = *std::get_if<JobNode::Subflow>(&node->handle_);
		auto& graph = handle.subgraph_;

		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			JobSubflow subflow(*this, worker, node, graph);

#if !SC_DISABLE_EXCEPTION_HANDLING
			try
			{
#endif
				handle.work_(subflow);
#if !SC_DISABLE_EXCEPTION_HANDLING
			}
			catch (...)
			{
				ProcessException(worker, node);
			}
#endif

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

		if ((node->nstate_ & JobNodeState::RETAIN_SUBFLOW) == 0)
		{
			graph.clear();
		}

		return false;
	}

	Bool JobExecutor::InvokeOwnedModuleTask(JobWorker& worker, JobNode* node)
	{
		return InvokeModuleTaskImplementation(worker, node, std::get_if<JobNode::OwnedModule>(&node->handle_)->graph_);
	}

	Bool JobExecutor::InvokeAdoptedModuleTask(JobWorker& worker, JobNode* node)
	{
		return InvokeModuleTaskImplementation(worker, node, std::get_if<JobNode::AdoptedModule>(&node->handle_)->graph_);
	}

	Bool JobExecutor::InvokeModuleTaskImplementation(JobWorker& worker, JobNode* node, JobGraph& graph)
	{
		if (graph.empty())
		{
			return false;
		}

		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			node->nstate_ |= JobNodeState::PREEMPTED;
			ScheduleGraph(worker, graph, node->topology_, node);
			return true;
		}

		node->nstate_ &= ~JobNodeState::PREEMPTED;

		return false;
	}

	Bool JobExecutor::InvokePreemptiveRuntimeTask(JobWorker& worker, JobNode* node)
	{
		return InvokeRuntimeTaskImplementation(worker, node, std::get_if<JobNode::PreemptiveRuntime>(&node->handle_)->work_);
	}

	Bool JobExecutor::InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&)>& function)
	{
		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
			JobPreemptiveRuntime runtime(*this, worker, node);

			node->nstate_ |= (JobNodeState::PREEMPTED | JobNodeState::IMPLICITLY_ANCHORED);

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

		return false;
	}

	Bool JobExecutor::InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&, Bool)>& function)
	{
		JobPreemptiveRuntime runtime(*this, worker, node);

		if ((node->nstate_ & JobNodeState::PREEMPTED) == 0)
		{
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

		function(runtime, true);

		return false;
	}

	void JobExecutor::ProcessException(JobWorker& worker, JobNode* node)
	{
		JobNodeBase* explicitAnchor = node;
		JobNodeBase* implicitAnchor = nullptr;

		while (explicitAnchor && (explicitAnchor->estate_.load(std::memory_order_relaxed) & JobExceptionState::EXPLICITLY_ANCHORED) == 0)
		{
			explicitAnchor->estate_.fetch_or(JobExceptionState::EXCEPTION, std::memory_order_relaxed);
			if (implicitAnchor == nullptr && (explicitAnchor->nstate_ & JobNodeState::IMPLICITLY_ANCHORED))
			{
				implicitAnchor = explicitAnchor;
			}
			explicitAnchor = explicitAnchor->parent_;
		}

		SC_CONSTEX auto flag = JobExceptionState::EXCEPTION | JobExceptionState::CAUGHT;

		if (explicitAnchor)
		{
			if ((explicitAnchor->estate_.fetch_or(flag, std::memory_order_relaxed) & JobExceptionState::CAUGHT) == 0)
			{
				explicitAnchor->exceptionPtr_ = std::current_exception();
				return;
			}
		}
		else if (implicitAnchor)
		{
			if ((implicitAnchor->estate_.fetch_or(flag, std::memory_order_relaxed) & JobExceptionState::CAUGHT) == 0)
			{
				implicitAnchor->exceptionPtr_ = std::current_exception();
				return;
			}
		}

		node->exceptionPtr_ = std::current_exception();
	}

	void JobExecutor::UpdateCache(JobWorker& worker, JobNode*& cache, JobNode* node)
	{
		if (cache)
		{
			Schedule(worker, cache);
		}
		cache = node;
	}

	void JobExecutor::CorunGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent)
	{
		if (graph.empty())
		{
			return;
		}

		{
			JobExplicitAnchorGuard anchor(parent);

			ScheduleGraph(worker, graph, topology, parent);
			CorunUntil(worker, [parent]()->Bool {return parent->joinCounter_.load(std::memory_order_acquire) == 0;});
		}

		parent->RethrowException();
	}

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

		notifier_.prepare_wait(worker.id_);

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

		for (Size buffer = 0;buffer < buffers_.size();++buffer)
		{
			if (!buffers_[buffer].queue_.empty())
			{
				notifier_.cancel_wait(worker.id_);
				worker.stickyVictim_ = buffer + workers_.size();
				goto exploreTask;
			}
		}

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

		notifier_.commit_wait(worker.id_);
		goto exploreTask;
	}
}
