#pragma once

namespace SeedCore
{
	class Semaphore
	{
	private:
		friend class JobNode;

	public:
		Semaphore() = default;

		explicit Semaphore(Size maxValue);

		Size value()const;

		Size max_value()const;

		void reset();

		void reset(Size newMaxValue);

	private:
		mutable std::mutex mutex_;

		Size maxValue_ = 0;

		Size currentValue_ = 0;

		HybridArray<JobNode*> waiters_;

		Bool try_acquire_or_wait(JobNode* node);

		void release(HybridArray<JobNode*>& nodes);
	};
}