#include <FoundationEngine/Coroutine/CoroutineSystem.h>

namespace SeedCore
{
	DynamicArray<std::coroutine_handle<Coroutine::promise_type>> CoroutineSystem::live_;
	DynamicArray<TimeEntry> CoroutineSystem::timer_;
	DynamicArray<FrameEntry> CoroutineSystem::frame_;
	DynamicArray<std::coroutine_handle<Coroutine::promise_type>> CoroutineSystem::ready_;
	DynamicArray<std::coroutine_handle<Coroutine::promise_type>> CoroutineSystem::resuming_;

	void CoroutineSystem::Update(Float deltaTime)
	{
		for (TimeEntry& entry : timer_)
		{
			entry.remainingTime_ -= deltaTime;

			if (entry.remainingTime_ <= 0.0f)
			{
				ready_.push_back(entry.handle_);
			}
		}

		erase_if(timer_, [](const TimeEntry& entry) { return entry.remainingTime_ <= 0.0f; });

		for (FrameEntry& entry : frame_)
		{
			--entry.remainingFrame_;

			if (entry.remainingFrame_ <= 0)
			{
				ready_.push_back(entry.handle_);
			}
		}

		erase_if(frame_, [](const FrameEntry& entry) { return entry.remainingFrame_ <= 0; });

		resuming_.swap(ready_);

		for (Size index = 0; index < resuming_.size(); ++index)
		{
			std::coroutine_handle<Coroutine::promise_type> handle = resuming_[index];

			if (!handle)
			{
				continue;
			}

			resuming_[index] = nullptr;
			handle.promise().manage_.suspended_ = false;

			handle.resume();
		}

		resuming_.clear();
	}

	void CoroutineSystem::Cancel(EntityID entity)
	{
		DynamicArray<std::coroutine_handle<Coroutine::promise_type>> targets;
		for (std::coroutine_handle<Coroutine::promise_type> handle : live_)
		{
			const Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			if (manage.entityID_ != entity)
			{
				continue;
			}

			if (manage.cancelled_)
			{
				continue;
			}

			targets.push_back(handle);
		}

		for (std::coroutine_handle<Coroutine::promise_type> handle : targets)
		{
			Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			if (!manage.suspended_)
			{
				manage.cancelled_ = true;
				continue;
			}

			erase_if(timer_, [handle](const TimeEntry& entry) { return entry.handle_ == handle; });
			erase_if(frame_, [handle](const FrameEntry& entry) { return entry.handle_ == handle; });
			erase_if(ready_, [handle](std::coroutine_handle<Coroutine::promise_type> entry) { return entry == handle; });

			for (std::coroutine_handle<Coroutine::promise_type>& entry : resuming_)
			{
				if (entry == handle)
				{
					entry = nullptr;
				}
			}

			handle.destroy();
		}
	}

	void CoroutineSystem::Cancel(EntityID entity, ComponentID component)
	{
		DynamicArray<std::coroutine_handle<Coroutine::promise_type>> targets;
		for (std::coroutine_handle<Coroutine::promise_type> handle : live_)
		{
			const Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			if (manage.entityID_ != entity || manage.componentID_ != component)
			{
				continue;
			}

			if (manage.cancelled_)
			{
				continue;
			}

			targets.push_back(handle);
		}

		for (std::coroutine_handle<Coroutine::promise_type> handle : targets)
		{
			Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			if (!manage.suspended_)
			{
				manage.cancelled_ = true;
				continue;
			}

			erase_if(timer_, [handle](const TimeEntry& entry) { return entry.handle_ == handle; });
			erase_if(frame_, [handle](const FrameEntry& entry) { return entry.handle_ == handle; });
			erase_if(ready_, [handle](std::coroutine_handle<Coroutine::promise_type> entry) { return entry == handle; });

			for (std::coroutine_handle<Coroutine::promise_type>& entry : resuming_)
			{
				if (entry == handle)
				{
					entry = nullptr;
				}
			}

			handle.destroy();
		}
	}

	void CoroutineSystem::ScheduleSeconds(std::coroutine_handle<Coroutine::promise_type> handle, Float seconds)
	{
		Coroutine::promise_type::Manage& manage = handle.promise().manage_;

		if (manage.cancelled_)
		{
			handle.destroy();
			return;
		}

		manage.suspended_ = true;
		timer_.push_back({ handle, seconds });
	}

	void CoroutineSystem::ScheduleFrames(std::coroutine_handle<Coroutine::promise_type> handle, Int frames)
	{
		Coroutine::promise_type::Manage& manage = handle.promise().manage_;

		if (manage.cancelled_)
		{
			handle.destroy();
			return;
		}

		manage.suspended_ = true;
		frame_.push_back({ handle, frames });
	}

	void CoroutineSystem::Register(std::coroutine_handle<Coroutine::promise_type> handle)
	{
		live_.push_back(handle);
	}

	void CoroutineSystem::Unregister(std::coroutine_handle<Coroutine::promise_type> handle)
	{
		auto it = std::find(live_.begin(), live_.end(), handle);
		if (it == live_.end())
		{
			return;
		}

		*it = live_.back();
		live_.pop_back();
	}
}
