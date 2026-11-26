#pragma once

namespace SeedCore
{
	struct JobNodeState
	{
		using NodeStateType = Uint32;

		SC_CONSTEX NodeStateType NONE = 0x00000000;

		SC_CONSTEX NodeStateType IMPLICITLY_ANCHORED = 0x10000000;

		SC_CONSTEX NodeStateType PREEMPTED = 0x20000000;

		SC_CONSTEX NodeStateType RETAIN_SUBFLOW = 0x40000000;

		SC_CONSTEX NodeStateType JOINED_SUBFLOW = 0x80000000;

		SC_CONSTEX NodeStateType STRONG_DEPENDENCIES_MASK = 0x0FFFFFFF;
	};

	using NState = JobNodeState::NodeStateType;

	struct JobExceptionState
	{
		using ExceptionStateType = Uint32;

		SC_CONSTEX ExceptionStateType NONE = 0x00000000;

		SC_CONSTEX ExceptionStateType EXCEPTION = 0x10000000;

		SC_CONSTEX ExceptionStateType CAUGHT = 0x20000000;

		SC_CONSTEX ExceptionStateType CANCELLED = 0x40000000;

		SC_CONSTEX ExceptionStateType EXPLICITLY_ANCHORED = 0x80000000;

		SC_CONSTEX ExceptionStateType LOCKED = 0x01000000;

		SC_CONSTEX ExceptionStateType FINISHED = 0x02000000;

		SC_CONSTEX ExceptionStateType MASK = 0xFF000000;
	};

	using EState = JobExceptionState::ExceptionStateType;

	class SEEDCORE_API JobNodeBase
	{
	private:
		friend class JobNode;
		friend class JobGraph;
		friend class JobTask;
		friend class JobTaskView;
		friend class JobTaskflow;
		friend class JobExecutor;
		friend class FlowBuilder;
		friend class JobSubflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;
		friend class JobExplicitAnchorGuard;

	protected:
		NState nstate_ = JobNodeState::NONE;

		std::atomic<EState> estate_ = JobExceptionState::NONE;

		JobNodeBase* parent_ = nullptr;

		std::atomic<Size> joinCounter_ = 0;

		std::exception_ptr exceptionPtr_ = nullptr;

	protected:
		JobNodeBase() = default;

		JobNodeBase(NState nstate, EState estate, JobNodeBase* parent, Size joinCounter);

		void RethrowException();
	};

	class SEEDCORE_API JobExplicitAnchorGuard
	{
	public:
		explicit JobExplicitAnchorGuard(JobNodeBase* node);

		~JobExplicitAnchorGuard();

		JobExplicitAnchorGuard(const JobExplicitAnchorGuard&) = delete;
		JobExplicitAnchorGuard& operator=(const JobExplicitAnchorGuard&) = delete;

	private:
		JobNodeBase* node_ = nullptr;
	};
}