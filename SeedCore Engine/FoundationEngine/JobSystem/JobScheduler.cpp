#include <FoundationEngine/JobSystem/JobScheduler.h>

namespace SeedCore
{
	void JobScheduler::SchedulerPrologue(JobWorker& worker)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	}

	void JobScheduler::SchedulerEpilogue(JobWorker& worker, std::exception_ptr)
	{
		CoUninitialize();
	}
}