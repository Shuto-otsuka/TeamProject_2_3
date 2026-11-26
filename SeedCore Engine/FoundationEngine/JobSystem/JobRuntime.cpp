#include <FoundationEngine/JobSystem/JobRuntime.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>

namespace SeedCore
{
	JobPreemptiveRuntime::JobPreemptiveRuntime(JobExecutor& executor, JobWorker& worker, JobNode* node) : executor_(executor), worker_(worker), node_(node)
	{
		/// No Code
	}

	JobExecutor& JobPreemptiveRuntime::Executor()
	{
		return executor_;
	}

	JobWorker& JobPreemptiveRuntime::Worker()
	{
		return worker_;
	}

	void JobPreemptiveRuntime::Schedule(JobTask task)
	{
		auto node = task.node_;

		node->joinCounter_.store(0, std::memory_order_relaxed);

		auto& join = node->parent_ ? node->parent_->joinCounter_ : node->topology_->joinCounter_;
		join.fetch_add(1, std::memory_order_relaxed);
		executor_.Schedule(worker_, node);
	}

	void JobPreemptiveRuntime::Corun()
	{
		{
			JobExplicitAnchorGuard anchor(node_);

			executor_.CorunUntil(worker_, [this]()->Bool {return node_->joinCounter_.load(std::memory_order_acquire) == 1;});
		}

		node_->RethrowException();
	}

	void JobPreemptiveRuntime::CorunAll()
	{
		Corun();
	}

	Bool JobPreemptiveRuntime::Cancelled()
	{
		return node_->ParentCancelled();
	}
}

namespace SeedCore
{
	JobNonpreemptiveRuntime::JobNonpreemptiveRuntime(JobExecutor& executor, JobWorker& worker) :executor_(executor), worker_(worker)
	{
		/// No Code
	}

	void JobNonpreemptiveRuntime::Schedule(JobTask task)
	{
		auto node = task.node_;

		node->joinCounter_.store(0, std::memory_order_relaxed);

		auto& join = node->parent_ ? node->parent_->joinCounter_ : node->topology_->joinCounter_;
		join.fetch_add(1, std::memory_order_relaxed);
		executor_.Schedule(worker_, node);
	}
}
