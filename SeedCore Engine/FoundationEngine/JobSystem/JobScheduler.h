#pragma once
#include <FoundationEngine/JobSystem/JobWorker.h>

namespace SeedCore
{
	class BenchmarkSystem;

	class JobScheduler :public JobWorkerInterface
	{
	public:
		/**
		* [EN]
		* Keeps the benchmark every worker thread attaches to when it starts.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各ワーカースレッドが起動時に Attach するベンチマークを保持する。
		*/
		JobScheduler(BenchmarkSystem& benchmark);

		void SchedulerPrologue(JobWorker& worker)override;

		void SchedulerEpilogue(JobWorker& worker, std::exception_ptr)override;

	private:
		/// [EN] Benchmark the worker threads record their sections into.
		/// [JP] ワーカースレッドが区間を記録するベンチマーク。
		BenchmarkSystem& benchmark_;
	};
}