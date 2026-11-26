#pragma once
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

	class SEEDCORE_API JobExecutor
	{
	private:
		friend class FlowBuilder;
		friend class JobSubflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;

	public:
		explicit JobExecutor(Size n = std::thread::hardware_concurrency(), ResourceRef<JobWorkerInterface> worker = nullptr);

		~JobExecutor();

		JobFuture<void> Run(JobTaskflow& taskflow);

		JobFuture<void> Run(JobTaskflow&& taskflow);

		template<typename C>
		JobFuture<void> Run(JobTaskflow& taskflow, C&& callable)
		{
			return RunNumber(taskflow, 1, std::forward<C>(callable));
		}

		template<typename C>
		JobFuture<void> Run(JobTaskflow&& taskflow, C&& callable)
		{
			return RunNumber(std::move(taskflow), 1, std::forward<C>(callable));
		}

		JobFuture<void> RunNumber(JobTaskflow& taskflow, Size n);

		JobFuture<void> RunNumber(JobTaskflow&& taskflow, Size n);

		template<typename C>
		JobFuture<void> RunNumber(JobTaskflow& taskflow, Size n, C&& callable)
		{
			return RunUntil(taskflow, [n]() mutable {return n-- == 0;}, std::forward<C>(callable));
		}

		template<typename C>
		JobFuture<void> RunNumber(JobTaskflow&& taskflow, Size n, C&& callable)
		{
			return RunUntil(std::move(taskflow), [n]() mutable {return n-- == 0;}, std::forward<C>(callable));
		}

		template<typename P>
		JobFuture<void> RunUntil(JobTaskflow& taskflow, P&& predicate)
		{
			return RunUntil(taskflow, std::forward<P>(predicate), []() {});
		}

		template<typename P>
		JobFuture<void> RunUntil(JobTaskflow&& taskflow, P&& predicate)
		{
			return RunUntil(std::move(taskflow), std::forward<P>(predicate), []() {});
		}

		template<typename P, typename C>
		JobFuture<void> RunUntil(JobTaskflow& taskflow, P&& predicate, C&& callable)
		{
			if (taskflow.Empty() || predicate())
			{
				callable();
				std::promise<void> promise;
				promise.set_value();
				return JobFuture<void>(promise.get_future());
			}

			IncrementTopology();

			ResourceRef<JobTopology> topology = MakeRef<JobTopology>(taskflow, std::forward<P>(predicate), std::forward<C>(callable));

			JobFuture<void> future(topology->promise_.get_future(), MakeObserve(topology));

			if (taskflow.FetchEnqueue(topology) == 0)
			{
				SetUpTopology(ThisWorker(), topology.get());
			}

			return future;
		}

		template<typename P, typename C>
		JobFuture<void> RunUntil(JobTaskflow&& taskflow, P&& predicate, C&& callable)
		{
			if (taskflow.Empty() || predicate())
			{
				callable();
				std::promise<void> promise;
				promise.set_value();
				return JobFuture<void>(promise.get_future());
			}

			IncrementTopology();

			ResourcePtr<JobTaskflow> ownedTaskflow = MakePtr<JobTaskflow>(std::move(taskflow));
			JobTaskflow& owned = *ownedTaskflow;

			ResourceRef<JobTopology> topology = MakeRef<JobTopology>(owned, std::forward<P>(predicate), std::forward<C>(callable));
			topology->ownedTaskflow_ = std::move(ownedTaskflow);

			JobFuture<void> future(topology->promise_.get_future(), MakeObserve(topology));

			if (owned.FetchEnqueue(topology) == 0)
			{
				SetUpTopology(ThisWorker(), topology.get());
			}

			return future;
		}

		template<typename T>
		void Corun(T& target)
		{
			JobWorker* worker = ThisWorker();
			if (worker == nullptr)
			{
				SC_THROW("Corun は JobExecutor のワーカーから呼び出す必要があります。");
			}

			JobNodeBase anchor;
			CorunGraph(*worker, RetrieveGraph(target), nullptr, &anchor);
		}

		template<typename P>
		void CorunUntil(P&& predicate)
		{
			JobWorker* worker = ThisWorker();
			if (worker == nullptr)
			{
				SC_THROW("CorunUntil は JobExecutor のワーカーから呼び出す必要があります。");
			}

			CorunUntil(*worker, std::forward<P>(predicate));
		}

		void WaitForAll();

		Size NumberWorkers()const noexcept;

		Size NumberWaiters()const noexcept;

		Size NumberQueues()const noexcept;

		Size NumberTopologies()const noexcept;

		JobWorker* ThisWorker();

		Int ThisWorkerID()const;

	private:
		struct Buffer
		{
			std::mutex mutex_;

			UnboundedWorkerQueue<JobNode*> queue_;
		};

		DynamicArray<JobWorker> workers_;

		DynamicArray<Buffer> buffers_;

		alignas(SC_CACHELINE_SIZE)DefaultNotifier notifier_;

		alignas(SC_CACHELINE_SIZE)std::atomic<Size> numberTopologies_{ 0 };

		std::unordered_map<std::thread::id, JobWorker*> thread2Worker_;

	private:
		void Shutdown();

		void Spawn(Size n, ResourceRef<JobWorkerInterface> worker);

		void ExploitTask(JobWorker& worker, JobNode*& cache);

		Bool ExploreTask(JobWorker& worker, JobNode*& cache);

		void Schedule(JobWorker& worker, JobNode*& cache);

		void Schedule(JobNode*& cache);

		void ScheduleGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent);

		void Spill(JobNode* node);

		void SetUpTopology(JobWorker* worker, JobTopology* topology);

		Size SetUpGraph(JobGraph& graph, JobTopology* topology, JobNodeBase* parent);

		void TearDownTopology(JobWorker& worker, JobTopology* topology, JobNode*& cache);

		void TearDownNonasync(JobWorker& worker, JobNode* node, JobNode*& cache);

		void TearDownInvoke(JobWorker& worker, JobNode* node, JobNode*& cache);

		void IncrementTopology();

		void DecrementTopology();

		void Invoke(JobWorker& worker, JobNode* node);

		void InvokeStaticTask(JobWorker& worker, JobNode* node);

		void InvokeNonpreemptiveRuntimeTask(JobWorker& worker, JobNode* node);

		void InvokeSingleConditionTask(JobWorker& worker, JobNode* node, HybridArray<Int>& conds);

		void InvokeMultiConditionTask(JobWorker& worker, JobNode* node, HybridArray<Int>& conds);

		Bool InvokeSubflowTask(JobWorker& worker, JobNode* node);

		Bool InvokeOwnedModuleTask(JobWorker& worker, JobNode* node);

		Bool InvokeAdoptedModuleTask(JobWorker& worker, JobNode* node);

		Bool InvokeModuleTaskImplementation(JobWorker& worker, JobNode* node, JobGraph& graph);

		Bool InvokePreemptiveRuntimeTask(JobWorker& worker, JobNode* node);

		Bool InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&)>& function);

		Bool InvokeRuntimeTaskImplementation(JobWorker& worker, JobNode* node, std::function<void(JobPreemptiveRuntime&, Bool)>& function);

		void ProcessException(JobWorker& worker, JobNode* node);

		void UpdateCache(JobWorker& worker, JobNode*& cache, JobNode* node);

		void CorunGraph(JobWorker& worker, JobGraph& graph, JobTopology* topology, JobNodeBase* parent);

		Bool WaitForTask(JobWorker& worker, JobNode*& cache);

		template<typename P>
		void CorunUntil(JobWorker& worker, P&& stopPredicate)
		{
			const Size MAX_VICTIM = NumberQueues();
			const Size MAX_STEALS = ((MAX_VICTIM + 1) << 1);

		exploit:

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

					t = (victim < workers_.size()) ? workers_[victim].wsq_.steal() : buffers_[victim - workers_.size()].queue_.steal();

					if (t)
					{
						Invoke(worker, t);
						worker.stickyVictim_ = victim;
						goto exploit;
					}
					else if (!stopPredicate())
					{
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

		template<typename I>
		void BulkSchedule(JobWorker& worker, I first, Size n)
		{
			if (n == 0)
			{
				return;
			}

			if (auto num = worker.wsq_.try_bulk_push(first, n);num != n)
			{
				BulkSpill(first, n - num);
			}

			notifier_.notify_count(n);
		}

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

		template<typename I>
		void BulkSpill(I first, Size n)
		{
			auto buffer = ((reinterpret_cast<uintptr_t>(*first) * 2654435761ULL) >> 32) % buffers_.size();
			std::scoped_lock lock(buffers_[buffer].mutex_);
			buffers_[buffer].queue_.bulk_push(first, n);
		}

		template<typename I>
		void BulkSpillRoundRobin(I first, Size n)
		{
			const Size buffer = buffers_.size();
			const Size start = ((reinterpret_cast<uintptr_t>(*first) * 2654435761ULL) >> 32) % buffer;
			const Size perBuffer = (n + buffer - 1) / buffer;
			Size remaining = n;
			for (Size index = 0;index < buffer && remaining>0;++index)
			{
				Size b = (start + index) % buffer;
				Size chunk = Min(perBuffer, remaining);

				{
					std::scoped_lock lock(buffers_[b].mutex_);
					buffers_[b].queue_.bulk_push(first, chunk);
				}

				remaining -= chunk;
			}
		}

		template<Size N>
		void BulkUpdateCache(JobWorker& worker, JobNode*& cache, JobNode* node, StaticArray<JobNode*, N>& array, Size& n)
		{
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
