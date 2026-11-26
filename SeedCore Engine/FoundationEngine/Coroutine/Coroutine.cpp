#include <FoundationEngine/Coroutine/Coroutine.h>
#include <FoundationEngine/Coroutine/CoroutineSystem.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	Coroutine::promise_type::~promise_type()
	{
		CoroutineSystem::Unregister(std::coroutine_handle<promise_type>::from_promise(*this));
	}

	Coroutine Coroutine::promise_type::get_return_object()
	{
		CoroutineSystem::Register(std::coroutine_handle<promise_type>::from_promise(*this));

		return Coroutine();
	}

	std::suspend_never Coroutine::promise_type::initial_suspend()noexcept
	{
		return {};
	}

	std::suspend_never Coroutine::promise_type::final_suspend()noexcept
	{
		return {};
	}

	void Coroutine::promise_type::return_void()noexcept
	{
		/// No Code
	}

	void Coroutine::promise_type::unhandled_exception()
	{
		try
		{
			std::rethrow_exception(std::current_exception());
		}
		catch(const Exception& e)
		{
			SC_THROW("Coroutine 内で未処理の例外が発生しました。: {}", e.what());
		}
		catch(const std::exception& e)
		{
			SC_THROW("Coroutine 内で未処理の例外が発生しました。: {}", e.what());
		}
		catch(...)
		{
			SC_THROW("Coroutine 内で未処理の不明な例外が発生しました。");
		}
	}
}

namespace SeedCore
{
	Bool Coroutine::SecondsAwaiter::await_ready()const noexcept
	{
		return seconds_ <= 0.0f;
	}

	void Coroutine::SecondsAwaiter::await_suspend(std::coroutine_handle<promise_type> handle)
	{
		CoroutineSystem::ScheduleSeconds(handle, seconds_);
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

	void Coroutine::FramesAwaiter::await_suspend(std::coroutine_handle<promise_type> handle)
	{
		CoroutineSystem::ScheduleFrames(handle, frames_);
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
