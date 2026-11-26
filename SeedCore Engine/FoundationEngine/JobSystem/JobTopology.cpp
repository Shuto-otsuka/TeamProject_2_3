#include <FoundationEngine/JobSystem/JobTopology.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	JobTopology::~JobTopology()
	{
		/// No Code
	}

	Bool JobTopology::Cancelled()const
	{
		return estate_.load(std::memory_order_relaxed) & (JobExceptionState::CANCELLED | JobExceptionState::EXCEPTION);
	}

	void JobTopology::CarryOutPromise()
	{
		if (exceptionPtr_)
		{
			auto exception = exceptionPtr_;
			exceptionPtr_ = nullptr;
			promise_.set_exception(exception);
		}
		else
		{
			promise_.set_value();
		}
	}
}
