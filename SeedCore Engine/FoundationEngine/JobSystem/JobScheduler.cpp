#include <FoundationEngine/JobSystem/JobScheduler.h>
#include <FoundationEngine/Benchmark/BenchmarkSystem.h>

namespace SeedCore
{
	JobScheduler::JobScheduler(BenchmarkSystem& benchmark) :benchmark_(benchmark)
	{
		/// No Code
	}

	void JobScheduler::SchedulerPrologue(JobWorker& worker)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		benchmark_.Attach(String(std::format("Worker {}", worker.ID())));
	}

	void JobScheduler::SchedulerEpilogue(JobWorker& worker, std::exception_ptr)
	{
		benchmark_.Detach();
		CoUninitialize();
	}
}