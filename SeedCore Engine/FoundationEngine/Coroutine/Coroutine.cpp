#include <FoundationEngine/Coroutine/Coroutine.h>
#include <FoundationEngine/Coroutine/CoroutineSystem.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	/**
	* [EN]
	* Unregisters the coroutine from CoroutineSystem. Runs whenever the
	* frame is freed, whether the body finished or it was destroyed while
	* suspended.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コルーチンを CoroutineSystem から登録解除する。本体が終わったときも、
	* 中断中に破棄されたときも、フレームが解放されるときに必ず実行される。
	*/
	Coroutine::promise_type::~promise_type()
	{
		CoroutineSystem::Unregister(std::coroutine_handle<promise_type>::from_promise(*this));
	}

	/**
	* [EN]
	* Registers the coroutine with CoroutineSystem and returns the
	* Coroutine token.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コルーチンを CoroutineSystem に登録し、Coroutine の印を返す。
	*/
	Coroutine Coroutine::promise_type::get_return_object()
	{
		/// [EN] Registered before the body starts, so a Cancel issued from inside the body already finds it.
		/// [JP] 本体が始まる前に登録するので、本体の中から出された Cancel でも見つけられる。
		CoroutineSystem::Register(std::coroutine_handle<promise_type>::from_promise(*this));

		return Coroutine();
	}

	/**
	* [EN]
	* Starts the body immediately, like calling an ordinary function.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 普通の関数呼び出しと同じように、本体をすぐに開始する。
	*/
	std::suspend_never Coroutine::promise_type::initial_suspend()noexcept
	{
		return {};
	}

	/**
	* [EN]
	* Does not suspend at the end, so the frame frees itself as soon as
	* the body finishes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最後に中断しないので、本体が終わるとすぐにフレームが自分で解放される。
	*/
	std::suspend_never Coroutine::promise_type::final_suspend()noexcept
	{
		return {};
	}

	/**
	* [EN]
	* Called when the body finishes; there is no return value.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 本体が終わったときに呼ばれる。戻り値は無い。
	*/
	void Coroutine::promise_type::return_void()noexcept
	{
		/// No Code
	}

	/**
	* [EN]
	* Rethrows an exception that escaped the body as an engine exception
	* with the original message.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 本体から漏れた例外を、元のメッセージ付きのエンジン例外として投げ直す。
	*/
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
	/**
	* [EN]
	* Returns true (no suspension) when there is nothing to wait for.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 待つ必要が無いときは true（中断しない）を返す。
	*/
	Bool Coroutine::SecondsAwaiter::await_ready()const noexcept
	{
		return seconds_ <= 0.0f;
	}

	/**
	* [EN]
	* Hands the suspended coroutine to CoroutineSystem's timer list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 中断したコルーチンを CoroutineSystem のタイマー一覧へ渡す。
	*/
	void Coroutine::SecondsAwaiter::await_suspend(std::coroutine_handle<promise_type> handle)
	{
		CoroutineSystem::ScheduleSeconds(handle, seconds_);
	}

	/**
	* [EN]
	* Called on resumption; there is no result.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 再開時に呼ばれる。結果は無い。
	*/
	void Coroutine::SecondsAwaiter::await_resume()const noexcept
	{
		/// No Code
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns true (no suspension) when there is nothing to wait for.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 待つ必要が無いときは true（中断しない）を返す。
	*/
	Bool Coroutine::FramesAwaiter::await_ready()const noexcept
	{
		return frames_ <= 0;
	}

	/**
	* [EN]
	* Hands the suspended coroutine to CoroutineSystem's frame list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 中断したコルーチンを CoroutineSystem のフレーム一覧へ渡す。
	*/
	void Coroutine::FramesAwaiter::await_suspend(std::coroutine_handle<promise_type> handle)
	{
		CoroutineSystem::ScheduleFrames(handle, frames_);
	}

	/**
	* [EN]
	* Called on resumption; there is no result.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 再開時に呼ばれる。結果は無い。
	*/
	void Coroutine::FramesAwaiter::await_resume()const noexcept
	{
		/// No Code
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns an awaiter that waits for seconds of scaled game time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイムスケール適用後のゲーム時間で seconds 秒待つ awaiter を返す。
	*/
	Coroutine::SecondsAwaiter Coroutine::WaitForSeconds(Float seconds)
	{
		return SecondsAwaiter{ seconds };
	}

	/**
	* [EN]
	* Returns an awaiter that waits for frames played frames.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プレイ中のフレームで frames フレーム待つ awaiter を返す。
	*/
	Coroutine::FramesAwaiter Coroutine::WaitForFrames(Int frames)
	{
		return FramesAwaiter{ frames };
	}
}
