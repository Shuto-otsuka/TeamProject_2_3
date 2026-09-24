#pragma once
#include <FoundationEngine/Prelude.h>

#define SC_COROUTINE(x) co_await (x)

namespace SeedCore
{
	class Coroutine
	{
	public:
		struct SecondsAwaiter
		{
			Float seconds_;

			Bool await_ready()const noexcept;

			void await_suspend(std::coroutine_handle<> handle);

			void await_resume()const noexcept;
		};

		struct FramesAwaiter
		{
			Int frames_;

			Bool await_ready()const noexcept;

			void await_suspend(std::coroutine_handle<> handle);

			void await_resume()const noexcept;
		};

	public:
		static SecondsAwaiter WaitForSeconds(Float seconds);

		static FramesAwaiter WaitForFrames(Int flames);
	};
}