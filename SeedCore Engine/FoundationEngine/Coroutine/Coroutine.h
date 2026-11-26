#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

#define SC_COROUTINE(x) co_await (x)

namespace SeedCore
{
	class SEEDCORE_API Coroutine
	{
	public:
		struct SEEDCORE_API promise_type
		{
			struct Manage
			{
				EntityID entityID_{};

				ComponentID componentID_ = nullptr;

				Bool suspended_ = false;

				Bool cancelled_ = false;
			};

			Manage manage_{};

			template<typename T, typename... Args>
			promise_type(T& component, Args&...)
			{
				manage_.entityID_ = ActorAccessor::GetEntity(component.GetActor()).GetID();
				manage_.componentID_ = ComponentRegistry::GetComponentID<std::remove_cv_t<std::remove_reference_t<T>>>();
			}

			~promise_type();

			Coroutine get_return_object();

			std::suspend_never initial_suspend()noexcept;

			std::suspend_never final_suspend()noexcept;

			void return_void()noexcept;

			void unhandled_exception();
		};

		struct SEEDCORE_API SecondsAwaiter
		{
			Float seconds_;

			Bool await_ready()const noexcept;

			void await_suspend(std::coroutine_handle<promise_type> handle);

			void await_resume()const noexcept;
		};

		struct SEEDCORE_API FramesAwaiter
		{
			Int frames_;

			Bool await_ready()const noexcept;

			void await_suspend(std::coroutine_handle<promise_type> handle);

			void await_resume()const noexcept;
		};

	public:
		Coroutine() = default;

		static SecondsAwaiter WaitForSeconds(Float seconds);

		static FramesAwaiter WaitForFrames(Int frames);
	};
}
