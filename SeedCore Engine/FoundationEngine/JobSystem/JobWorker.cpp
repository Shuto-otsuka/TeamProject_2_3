#include <FoundationEngine/JobSystem/JobWorker.h>
#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	Size JobWorker::ID()const
	{
		return id_;
	}

	Size JobWorker::QueueSize()const
	{
		return wsq_.size();
	}

	Size JobWorker::QueueCapacity()const
	{
		return static_cast<Size>(wsq_.capacity());
	}

	std::thread& JobWorker::Thread()
	{
		return thread_;
	}
}

namespace SeedCore
{
	JobWorkerView::JobWorkerView(const JobWorker& worker) :worker_(worker)
	{
		/// No Code
	}

	Size JobWorkerView::ID()const
	{
		return worker_.id_;
	}

	Size JobWorkerView::QueueSize()const
	{
		return worker_.wsq_.size();
	}

	Size JobWorkerView::QueueCapacity()const
	{
		return static_cast<Size>(worker_.wsq_.capacity());
	}
}
