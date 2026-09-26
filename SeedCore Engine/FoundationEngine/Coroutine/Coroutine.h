#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

#define SC_COROUTINE(x) co_await (x)

namespace SeedCore
{
	/**
	* [EN]
	* Return type of a component's coroutine member function. Calling the
	* function starts the body immediately and runs it up to its first
	* co_await; from then on CoroutineSystem resumes it. The frame frees
	* itself when the body finishes, and CoroutineSystem destroys it early
	* when its entity or component is removed, so this object is only a
	* token that the coroutine was started and owns nothing - discarding
	* it (e.g. `WaitTest();`) is the intended use.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネントのコルーチンメンバ関数の戻り値の型。関数を呼ぶと本体が
	* すぐに始まり、最初の co_await まで実行される。以降は CoroutineSystem が
	* 再開する。フレームは本体が終わると自分で解放され、エンティティや
	* コンポーネントが削除された場合は CoroutineSystem が途中で破棄する。
	* そのためこのオブジェクトは「コルーチンを開始した」という印にすぎず、
	* 何も所有しない。捨てて使う（例: `WaitTest();`）のが想定した使い方。
	*/
	class SEEDCORE_API Coroutine
	{
	public:
		/**
		* [EN]
		* Coroutine promise. Records which entity/component the coroutine
		* belongs to, so CoroutineSystem can cancel it when that component
		* goes away, and tracks whether it is currently suspended or has
		* been cancelled.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コルーチンの promise。コルーチンがどのエンティティ/コンポーネントに
		* 属するかを記録し、そのコンポーネントが無くなったときに
		* CoroutineSystem がキャンセルできるようにする。また、現在中断中か、
		* キャンセル済みかを管理する。
		*/
		struct SEEDCORE_API promise_type
		{
			/**
			* [EN]
			* Ownership and state of one coroutine, read by CoroutineSystem.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 1つのコルーチンの所属と状態。CoroutineSystem が参照する。
			*/
			struct Manage
			{
				/// [EN] Entity owning the component whose member function this coroutine is.
				/// [JP] このコルーチンをメンバ関数として持つコンポーネントの、所有元エンティティ。
				EntityID entityID_{};

				/// [EN] Component type whose member function this coroutine is.
				/// [JP] このコルーチンをメンバ関数として持つコンポーネントの型。
				ComponentID componentID_ = nullptr;

				/// [EN] True while suspended at a co_await (waiting in CoroutineSystem), false while the body is executing.
				/// [JP] co_await で中断中（CoroutineSystem で待機中）なら true、本体を実行中なら false。
				Bool suspended_ = false;

				/// [EN] Set when the owner is removed while the body is executing; the next co_await destroys the coroutine instead of waiting.
				/// [JP] 本体の実行中に所有者が削除されたときに立つ。次の co_await では待機せず、コルーチンを破棄する。
				Bool cancelled_ = false;
			};

			/// [EN] This coroutine's ownership and state.
			/// [JP] このコルーチンの所属と状態。
			Manage manage_{};

			/**
			* [EN]
			* Receives the arguments of the coroutine function. For a member
			* function the first one is the object itself, which gives the
			* owning entity and component type.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* コルーチン関数の引数を受け取る。メンバ関数の場合は先頭が
			* オブジェクト自身なので、そこから所有元のエンティティと
			* コンポーネントの型を得る。
			*/
			template<typename T, typename... Args>
			promise_type(T& component, Args&...)
			{
				manage_.entityID_ = component.GetActor().GetEntity().GetID();
				manage_.componentID_ = ComponentRegistry::GetComponentID<std::remove_cv_t<std::remove_reference_t<T>>>();
			}

			/**
			* [EN]
			* Unregisters the coroutine from CoroutineSystem. Runs whenever
			* the frame is freed, whether the body finished or it was
			* destroyed while suspended.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* コルーチンを CoroutineSystem から登録解除する。本体が終わった
			* ときも、中断中に破棄されたときも、フレームが解放されるときに
			* 必ず実行される。
			*/
			~promise_type();

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
			Coroutine get_return_object();

			/**
			* [EN]
			* Starts the body immediately, like calling an ordinary function.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 普通の関数呼び出しと同じように、本体をすぐに開始する。
			*/
			std::suspend_never initial_suspend()noexcept;

			/**
			* [EN]
			* Does not suspend at the end, so the frame frees itself as soon
			* as the body finishes.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 最後に中断しないので、本体が終わるとすぐにフレームが自分で
			* 解放される。
			*/
			std::suspend_never final_suspend()noexcept;

			/**
			* [EN]
			* Called when the body finishes; there is no return value.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 本体が終わったときに呼ばれる。戻り値は無い。
			*/
			void return_void()noexcept;

			/**
			* [EN]
			* Rethrows an exception that escaped the body as an engine
			* exception with the original message.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 本体から漏れた例外を、元のメッセージ付きのエンジン例外として
			* 投げ直す。
			*/
			void unhandled_exception();
		};

		/**
		* [EN]
		* Awaiter that suspends the coroutine for a number of seconds of
		* scaled game time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タイムスケール適用後のゲーム時間で、指定秒数だけコルーチンを中断する
		* awaiter。
		*/
		struct SEEDCORE_API SecondsAwaiter
		{
			/// [EN] Seconds to wait; 0 or less continues without suspending.
			/// [JP] 待つ秒数。0 以下なら中断せずにそのまま続ける。
			Float seconds_;

			/**
			* [EN]
			* Returns true (no suspension) when there is nothing to wait for.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 待つ必要が無いときは true（中断しない）を返す。
			*/
			Bool await_ready()const noexcept;

			/**
			* [EN]
			* Hands the suspended coroutine to CoroutineSystem's timer list.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 中断したコルーチンを CoroutineSystem のタイマー一覧へ渡す。
			*/
			void await_suspend(std::coroutine_handle<promise_type> handle);

			/**
			* [EN]
			* Called on resumption; there is no result.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 再開時に呼ばれる。結果は無い。
			*/
			void await_resume()const noexcept;
		};

		/**
		* [EN]
		* Awaiter that suspends the coroutine for a number of played frames.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プレイ中のフレーム数で、指定フレームだけコルーチンを中断する
		* awaiter。
		*/
		struct SEEDCORE_API FramesAwaiter
		{
			/// [EN] Frames to wait; 0 or less continues without suspending.
			/// [JP] 待つフレーム数。0 以下なら中断せずにそのまま続ける。
			Int frames_;

			/**
			* [EN]
			* Returns true (no suspension) when there is nothing to wait for.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 待つ必要が無いときは true（中断しない）を返す。
			*/
			Bool await_ready()const noexcept;

			/**
			* [EN]
			* Hands the suspended coroutine to CoroutineSystem's frame list.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 中断したコルーチンを CoroutineSystem のフレーム一覧へ渡す。
			*/
			void await_suspend(std::coroutine_handle<promise_type> handle);

			/**
			* [EN]
			* Called on resumption; there is no result.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 再開時に呼ばれる。結果は無い。
			*/
			void await_resume()const noexcept;
		};

	public:
		/**
		* [EN]
		* Creates the token; only get_return_object constructs one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 印を作る。これを構築するのは get_return_object だけ。
		*/
		Coroutine() = default;

		/**
		* [EN]
		* Returns an awaiter that waits for seconds of scaled game time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タイムスケール適用後のゲーム時間で seconds 秒待つ awaiter を返す。
		*/
		static SecondsAwaiter WaitForSeconds(Float seconds);

		/**
		* [EN]
		* Returns an awaiter that waits for frames played frames.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プレイ中のフレームで frames フレーム待つ awaiter を返す。
		*/
		static FramesAwaiter WaitForFrames(Int frames);
	};
}
