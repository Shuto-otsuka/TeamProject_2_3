#pragma once
#include <FoundationEngine/JobSystem/JobWorker.h>

namespace SeedCore
{
	class BenchmarkSystem;

	class JobScheduler :public JobWorkerInterface
	{
	public:
		JobScheduler(BenchmarkSystem& benchmark);

		void SchedulerPrologue(JobWorker& worker)override;

		void SchedulerEpilogue(JobWorker& worker, std::exception_ptr)override;

	private:
		BenchmarkSystem& benchmark_;
	};
}