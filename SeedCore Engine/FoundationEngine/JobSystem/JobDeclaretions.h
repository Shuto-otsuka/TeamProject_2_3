#pragma once
#include <FoundationEngine/JobSystem/WorkerCommon.h>

namespace SeedCore
{
	class JobNode;
	class JobGraph;
	class FlowBuilder;

	class Semaphore;

	class JobSubflow;
	class JobPreemptiveRuntime;
	class JobNonpreemptiveRuntime;

	class JobTask;
	class JobTaskView;
	class JobTaskflow;
	class JobTopology;

	class JobExecutor;
	class JobWorker;
	class JobWorkerView;

	template <typename T>
	class JobFuture;

	class TaskParams;
	class DefaultTaskParams;
}