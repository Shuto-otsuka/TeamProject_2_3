#include <FoundationEngine/JobSystem/JobScheduler.h>
#include <FoundationEngine/Benchmark/BenchmarkSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Keeps the benchmark every worker thread attaches to when it starts.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各ワーカースレッドが起動時に Attach するベンチマークを保持する。
	*/
	JobScheduler::JobScheduler(BenchmarkSystem& benchmark) :benchmark_(benchmark)
	{
		/// No Code
	}

	void JobScheduler::SchedulerPrologue(JobWorker& worker)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		/// [EN] Each worker gets its own timeline lane, named by its index in the pool.
		/// [JP] ワーカーごとにタイムラインのレーンを持たせ、プール内の番号で名付ける。
		benchmark_.Attach(String(std::format("Worker {}", worker.ID())));
	}

	void JobScheduler::SchedulerEpilogue(JobWorker& worker, std::exception_ptr)
	{
		benchmark_.Detach();
		CoUninitialize();
	}
}