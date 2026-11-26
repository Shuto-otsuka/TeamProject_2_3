#pragma once
#include <FoundationEngine/JobSystem/JobNodeBase.h>

namespace SeedCore
{
	class JobTopology :public JobNodeBase
	{
	private:
		friend class JobExecutor;
		friend class Subflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;
		friend class JobNode;

		template<typename T>
		friend class JobFuture;

	public:
		template<typename Predicate, typename OnFinish>
		JobTopology(JobTaskflow& taskflow, Predicate&& predicate, OnFinish&& onFinish) :JobNodeBase(JobNodeState::NONE, JobExceptionState::EXPLICITLY_ANCHORED, nullptr, 0), taskflow_(taskflow), predicate_(std::forward<Predicate>(predicate)), onFinish_(std::forward<OnFinish>(onFinish))
		{
			/// No Code
		}

		~JobTopology();

		Bool Cancelled()const;

	private:
		JobTaskflow& taskflow_;

		ResourcePtr<JobTaskflow> ownedTaskflow_;

		std::promise<void> promise_;

		std::function<Bool()> predicate_;

		std::function<void()> onFinish_;

		void CarryOutPromise();
	};
}
