#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Coroutine/Coroutine.h>

namespace SeedCore
{
	/**
	* [EN]
	* A coroutine waiting on WaitForSeconds, with the scaled game time
	* still left before it resumes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* WaitForSeconds で待機中のコルーチンと、再開までに残っている
	* タイムスケール適用後のゲーム時間。
	*/
	struct TimeEntry
	{
		/// [EN] The waiting coroutine.
		/// [JP] 待機中のコルーチン。
		std::coroutine_handle<Coroutine::promise_type> handle_;

		/// [EN] Seconds left; the coroutine becomes ready once this reaches 0 or less.
		/// [JP] 残り秒数。0 以下になるとコルーチンは再開待ちになる。
		Float remainingTime_;
	};

	/**
	* [EN]
	* A coroutine waiting on WaitForFrames, with the played frames still
	* left before it resumes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* WaitForFrames で待機中のコルーチンと、再開までに残っているプレイ中の
	* フレーム数。
	*/
	struct FrameEntry
	{
		/// [EN] The waiting coroutine.
		/// [JP] 待機中のコルーチン。
		std::coroutine_handle<Coroutine::promise_type> handle_;

		/// [EN] Frames left; the coroutine becomes ready once this reaches 0 or less.
		/// [JP] 残りフレーム数。0 以下になるとコルーチンは再開待ちになる。
		Int remainingFrame_;
	};

	/**
	* [EN]
	* Drives every component coroutine: keeps the waiting ones, resumes
	* them once their wait is over, and destroys them when their entity or
	* component is removed. Every live coroutine is registered here from
	* its start until its frame is freed, so Cancel reaches it wherever it
	* is - waiting on a timer or frame count, queued for resumption this
	* frame, or executing its body right now.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのコンポーネントのコルーチンを動かす: 待機中のものを保持し、待ちが
	* 終わったら再開し、エンティティやコンポーネントが削除されたら破棄する。
	* 生きているコルーチンは開始からフレームが解放されるまで全てここに
	* 登録されるので、Cancel はそれがどこにあっても届く - 時間やフレーム数を
	* 待っていても、このフレームの再開待ちに並んでいても、今まさに本体を
	* 実行中でも。
	*/
	class SEEDCORE_API CoroutineSystem
	{
	public:
		/**
		* [EN]
		* Advances every waiting coroutine by one played frame of
		* deltaTime seconds and resumes the ones whose wait is over. Call
		* once per frame, only while the game is playing and not paused,
		* so neither kind of wait advances outside of play.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 待機中の全コルーチンを、deltaTime 秒のプレイ中1フレーム分進め、
		* 待ちが終わったものを再開する。毎フレーム1回、ゲームがプレイ中かつ
		* ポーズしていない間だけ呼ぶこと。そうすればどちらの種類の待ちも
		* プレイ外では進まない。
		*/
		static void Update(Float deltaTime);

		/**
		* [EN]
		* Cancels every coroutine belonging to any component of entity. A
		* suspended one is taken out of the waiting lists and destroyed at
		* once; one that is executing cannot be destroyed under its own
		* feet, so it is only marked and destroyed at its next co_await.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity のいずれかのコンポーネントに属する全コルーチンをキャンセル
		* する。中断中のものは待機一覧から外してすぐに破棄する。実行中の
		* ものは実行している最中に破棄することはできないので、印だけ付けて
		* 次の co_await で破棄する。
		*/
		static void Cancel(EntityID entity);

		/**
		* [EN]
		* Cancels every coroutine belonging to entity's component of type
		* component. A suspended one is taken out of the waiting lists and
		* destroyed at once; one that is executing cannot be destroyed under
		* its own feet, so it is only marked and destroyed at its next
		* co_await.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity の、型が component のコンポーネントに属する全コルーチンを
		* キャンセルする。中断中のものは待機一覧から外してすぐに破棄する。
		* 実行中のものは実行している最中に破棄することはできないので、印だけ
		* 付けて次の co_await で破棄する。
		*/
		static void Cancel(EntityID entity, ComponentID component);

		/**
		* [EN]
		* Suspends handle for seconds of scaled game time. A coroutine that
		* was cancelled while executing is destroyed here instead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle をタイムスケール適用後のゲーム時間で seconds 秒中断させる。
		* 実行中にキャンセルされたコルーチンは、代わりにここで破棄する。
		*/
		static void ScheduleSeconds(std::coroutine_handle<Coroutine::promise_type> handle, Float seconds);

		/**
		* [EN]
		* Suspends handle for frames played frames. A coroutine that was
		* cancelled while executing is destroyed here instead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle をプレイ中の frames フレーム中断させる。実行中に
		* キャンセルされたコルーチンは、代わりにここで破棄する。
		*/
		static void ScheduleFrames(std::coroutine_handle<Coroutine::promise_type> handle, Int frames);

		/**
		* [EN]
		* Adds a coroutine that has just started to the live list.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 開始したばかりのコルーチンを生存一覧に加える。
		*/
		static void Register(std::coroutine_handle<Coroutine::promise_type> handle);

		/**
		* [EN]
		* Removes a coroutine whose frame is being freed from the live list.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレームが解放されようとしているコルーチンを生存一覧から取り除く。
		*/
		static void Unregister(std::coroutine_handle<Coroutine::promise_type> handle);

	private:
		/// [EN] Every coroutine that has started and whose frame has not been freed yet.
		/// [JP] 開始済みで、まだフレームが解放されていない全コルーチン。
		static DynamicArray<std::coroutine_handle<Coroutine::promise_type>> live_;

		/// [EN] Coroutines waiting on WaitForSeconds.
		/// [JP] WaitForSeconds で待機中のコルーチン。
		static DynamicArray<TimeEntry> timer_;

		/// [EN] Coroutines waiting on WaitForFrames.
		/// [JP] WaitForFrames で待機中のコルーチン。
		static DynamicArray<FrameEntry> frame_;

		/// [EN] Coroutines whose wait ended this frame, collected before any of them is resumed.
		/// [JP] このフレームで待ちが終わったコルーチン。どれかを再開する前に集めておく。
		static DynamicArray<std::coroutine_handle<Coroutine::promise_type>> ready_;

		/// [EN] This frame's batch being resumed; an entry is cleared when it is resumed or cancelled, so Cancel can still reach the ones not resumed yet.
		/// [JP] このフレームで再開中の一群。再開またはキャンセルされたエントリは空にするので、まだ再開していないものにも Cancel が届く。
		static DynamicArray<std::coroutine_handle<Coroutine::promise_type>> resuming_;
	};
}
