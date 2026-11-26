#pragma once
#include <FoundationEngine/JobSystem/JobDeclaretions.h>
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/JobSystem/WorkerQueue.h>
#include <FoundationEngine/JobSystem/AtomicNotifier.h>
#include <FoundationEngine/JobSystem/NonblockingNotifier.h>
#include <FoundationEngine/Math/Random/Xorshift.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
#if SC_ENABLE_ATOMIC_NOTIFIER
	using DefaultNotifier = AtomicNotifier;
#else
	using DefaultNotifier = NonblockingNotifier;
#endif

	class JobWorker
	{
	private:
		friend class JobExecutor;
		friend class JobPreemptiveRuntime;
		friend class JobWorkerView;

		using WsqType = BoundedWorkerQueue<JobNode*>;

	public:
		Size ID()const;

		Size QueueSize()const;

		Size QueueCapacity()const;

		std::thread& Thread();

	private:
		alignas(SC_CACHELINE_SIZE)std::atomic_flag done_{};

		Size id_;

		Size stickyVictim_;

		Xorshift<Uint32> rdgen_;

		std::thread thread_;

		WsqType wsq_;
	};

	class JobWorkerView
	{
	private:
		friend class JobExecutor;

	public:
		Size ID()const;

		Size QueueSize()const;

		Size QueueCapacity()const;

	private:
		JobWorkerView(const JobWorker& worker);

		JobWorkerView(const JobWorkerView&) = default;

		const JobWorker& worker_;
	};

	class JobWorkerInterface
	{
	public:
		virtual ~JobWorkerInterface() = default;

		virtual void SchedulerPrologue(JobWorker& worker) = 0;

		virtual void SchedulerEpilogue(JobWorker& worker, std::exception_ptr ptr) = 0;
	};

	template<typename T, typename... Args>
	ResourceRef<T> MakeWorkerInterface(Args&&... args)
	{
		return MakeRef<T>(std::forward<Args>(args)...);
	}
}
