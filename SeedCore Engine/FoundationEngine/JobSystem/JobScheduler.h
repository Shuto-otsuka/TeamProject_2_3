#pragma once
#include <FoundationEngine/JobSystem/JobWorker.h>

namespace SeedCore
{
	class JobScheduler :public JobWorkerInterface
	{
	public:
		void SchedulerPrologue(JobWorker& worker)override;

		void SchedulerEpilogue(JobWorker& worker, std::exception_ptr)override;
	};
}