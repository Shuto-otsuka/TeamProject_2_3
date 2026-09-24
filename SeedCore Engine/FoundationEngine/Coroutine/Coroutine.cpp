#include <FoundationEngine/Coroutine/Coroutine.h>

namespace SeedCore
{
	Bool Coroutine::SecondsAwaiter::await_ready()const noexcept
	{
		return seconds_ <= 0.0f;
	}

	void Coroutine::SecondsAwaiter::await_suspend(std::coroutine_handle<> handle)
	{

	}

	void Coroutine::SecondsAwaiter::await_resume()const noexcept
	{
		/// No Code
	}
}

namespace SeedCore
{
	Bool Coroutine::FramesAwaiter::await_ready()const noexcept
	{
		return frames_ <= 0;
	}

	void Coroutine::FramesAwaiter::await_suspend(std::coroutine_handle<> handle)
	{

	}

	void Coroutine::FramesAwaiter::await_resume()const noexcept
	{
		/// No Code
	}
}

namespace SeedCore
{
	Coroutine::SecondsAwaiter Coroutine::WaitForSeconds(Float seconds)
	{
		return SecondsAwaiter{ seconds };
	}

	Coroutine::FramesAwaiter Coroutine::WaitForFrames(Int frames)
	{
		return FramesAwaiter{ frames };
	}
}