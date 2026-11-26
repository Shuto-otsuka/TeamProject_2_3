#include <FoundationEngine/JobSystem/JobNodeBase.h>

namespace SeedCore
{
	JobNodeBase::JobNodeBase(NState nstate, EState estate, JobNodeBase* parent, Size joinCounter) :nstate_(nstate), estate_(estate), parent_(parent), joinCounter_(joinCounter)
	{
		/// No Code
	}

	void JobNodeBase::RethrowException()
	{
		if (exceptionPtr_)
		{
			std::exception_ptr exception = exceptionPtr_;
			exceptionPtr_ = nullptr;
			estate_.fetch_and(~(JobExceptionState::EXCEPTION | JobExceptionState::CAUGHT), std::memory_order_relaxed);
			std::rethrow_exception(exception);
		}
	}
}

namespace SeedCore
{
	JobExplicitAnchorGuard::JobExplicitAnchorGuard(JobNodeBase* node) :node_(node)
	{
		node_->estate_.fetch_or(JobExceptionState::EXPLICITLY_ANCHORED, std::memory_order_relaxed);
	}

	JobExplicitAnchorGuard::~JobExplicitAnchorGuard()
	{
		node_->estate_.fetch_and(~JobExceptionState::EXPLICITLY_ANCHORED, std::memory_order_relaxed);
	}
}