#pragma once
#include <FoundationEngine/JobSystem/JobTask.h>

namespace SeedCore
{
	class JobPreemptiveRuntime
	{
	private:
		friend class JobExecutor;
		friend class FlowBuilder;

	public:
		JobExecutor& Executor();

		JobWorker& Worker();

		void Schedule(JobTask task);

		void Corun();

		void CorunAll();

		Bool Cancelled();

	private:
		explicit JobPreemptiveRuntime(JobExecutor& executor, JobWorker& worker, JobNode* node);

		JobExecutor& executor_;

		JobWorker& worker_;

		JobNode* node_;
	};

	class JobNonpreemptiveRuntime
	{
	private:
		friend class JobExecutor;

	public:
		void Schedule(JobTask task);

	private:
		explicit JobNonpreemptiveRuntime(JobExecutor& executor, JobWorker& worker);

		JobExecutor& executor_;

		JobWorker& worker_;
	};
}
