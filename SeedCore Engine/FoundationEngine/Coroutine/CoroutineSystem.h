#pragma once
#include <FoundationEngine/Coroutine/Coroutine.h>

namespace SeedCore
{
	struct TimeEntry
	{
		std::coroutine_handle<Coroutine::promise_type> handle_;

		Float remainingTime_;
	};

	struct FrameEntry
	{
		std::coroutine_handle<Coroutine::promise_type> handle_;

		Int remainingFrame_;
	};

	class SEEDCORE_API CoroutineSystem
	{
	public:
		static void Update(Float deltaTime);

		static void Cancel(EntityID entity);

		static void Cancel(EntityID entity, ComponentID component);

		static void ScheduleSeconds(std::coroutine_handle<Coroutine::promise_type> handle, Float seconds);

		static void ScheduleFrames(std::coroutine_handle<Coroutine::promise_type> handle, Int frames);

		static void Register(std::coroutine_handle<Coroutine::promise_type> handle);

		static void Unregister(std::coroutine_handle<Coroutine::promise_type> handle);

	private:
		static DynamicArray<std::coroutine_handle<Coroutine::promise_type>> live_;

		static DynamicArray<TimeEntry> timer_;

		static DynamicArray<FrameEntry> frame_;

		static DynamicArray<std::coroutine_handle<Coroutine::promise_type>> ready_;

		static DynamicArray<std::coroutine_handle<Coroutine::promise_type>> resuming_;
	};
}
