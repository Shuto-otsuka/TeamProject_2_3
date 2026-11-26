#include <FoundationEngine/JobSystem/Semaphore.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	Semaphore::Semaphore(Size maxValue) :maxValue_(maxValue), currentValue_(maxValue)
	{
		/// No Code
	}

	Size Semaphore::value()const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return currentValue_;
	}

	Size Semaphore::max_value()const
	{
		return maxValue_;
	}

	void Semaphore::reset()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		currentValue_ = maxValue_;

		waiters_.clear();
	}

	void Semaphore::reset(Size newMaxValue)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		currentValue_ = (maxValue_ = newMaxValue);

		waiters_.clear();
	}

	Bool Semaphore::try_acquire_or_wait(JobNode* node)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (currentValue_ > 0)
		{
			--currentValue_;
			return true;
		}
		else
		{
			waiters_.push_back(node);
			return false;
		}
	}

	void Semaphore::release(HybridArray<JobNode*>& nodes)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (currentValue_ >= maxValue_)
		{
			SC_THROW("Semaphore::release() が acquire 済みの回数を超えて呼び出されました。");
		}

		++currentValue_;

		if (nodes.empty())
		{
			nodes.swap(waiters_);
		}
		else
		{
			nodes.reserve(nodes.size() + waiters_.size());
			nodes.insert(nodes.end(), waiters_.begin(), waiters_.end());
			waiters_.clear();
		}
	}
}