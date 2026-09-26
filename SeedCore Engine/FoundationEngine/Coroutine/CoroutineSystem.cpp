#include <FoundationEngine/Coroutine/CoroutineSystem.h>

namespace SeedCore
{
	DynamicArray<std::coroutine_handle<Coroutine::promise_type>> CoroutineSystem::live_;
	DynamicArray<TimeEntry> CoroutineSystem::timer_;
	DynamicArray<FrameEntry> CoroutineSystem::frame_;
	DynamicArray<std::coroutine_handle<Coroutine::promise_type>> CoroutineSystem::ready_;
	DynamicArray<std::coroutine_handle<Coroutine::promise_type>> CoroutineSystem::resuming_;

	/**
	* [EN]
	* Advances every waiting coroutine by one played frame of deltaTime
	* seconds and resumes the ones whose wait is over. Call once per
	* frame, only while the game is playing and not paused, so neither
	* kind of wait advances outside of play.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 待機中の全コルーチンを、deltaTime 秒のプレイ中1フレーム分進め、待ちが
	* 終わったものを再開する。毎フレーム1回、ゲームがプレイ中かつポーズして
	* いない間だけ呼ぶこと。そうすればどちらの種類の待ちもプレイ外では
	* 進まない。
	*/
	void CoroutineSystem::Update(Float deltaTime)
	{
		/// [EN] Count down the time waits; the finished ones move to ready_.
		/// [JP] 時間待ちを減らし、終わったものを ready_ へ移す。
		for (TimeEntry& entry : timer_)
		{
			entry.remainingTime_ -= deltaTime;

			if (entry.remainingTime_ <= 0.0f)
			{
				ready_.push_back(entry.handle_);
			}
		}

		erase_if(timer_, [](const TimeEntry& entry) { return entry.remainingTime_ <= 0.0f; });

		/// [EN] Count down the frame waits; the finished ones move to ready_.
		/// [JP] フレーム待ちを減らし、終わったものを ready_ へ移す。
		for (FrameEntry& entry : frame_)
		{
			--entry.remainingFrame_;

			if (entry.remainingFrame_ <= 0)
			{
				ready_.push_back(entry.handle_);
			}
		}

		erase_if(frame_, [](const FrameEntry& entry) { return entry.remainingFrame_ <= 0; });

		/// [EN] The ready ones become this frame's batch. A coroutine that waits again while being resumed goes to timer_/frame_, never into this batch, so it cannot run twice in one frame.
		/// [JP] 再開待ちのものをこのフレームの一群にする。再開中に再び待ったコルーチンは timer_/frame_ へ入り、この一群には入らないので、1フレームに2回動くことは無い。
		resuming_.swap(ready_);

		/// [EN] Walked by index and never by iterator: a resumed body may Cancel another entry of this batch, which clears that entry in place.
		/// [JP] イテレータではなく添字で回す。再開した本体がこの一群の別のエントリを Cancel することがあり、その場合そのエントリはその場で空にされる。
		for (Size index = 0; index < resuming_.size(); ++index)
		{
			std::coroutine_handle<Coroutine::promise_type> handle = resuming_[index];

			/// [EN] Cancelled by an earlier coroutine of this batch; already destroyed.
			/// [JP] この一群の先に再開したコルーチンによってキャンセル済み。既に破棄されている。
			if (!handle)
			{
				continue;
			}

			/// [EN] Leaves the batch before running, so from here on Cancel treats it as executing rather than waiting.
			/// [JP] 実行する前に一群から外す。ここから先、Cancel はこれを待機中ではなく実行中として扱う。
			resuming_[index] = nullptr;
			handle.promise().manage_.suspended_ = false;

			/// [EN] Runs until the next co_await (back into timer_/frame_) or the end of the body (the frame frees itself); either way the handle is not touched again here.
			/// [JP] 次の co_await（timer_/frame_ へ戻る）か本体の終わり（フレームは自分で解放される）まで動く。どちらの場合もここではもうハンドルに触れない。
			handle.resume();
		}

		resuming_.clear();
	}

	/**
	* [EN]
	* Cancels every coroutine belonging to any component of entity. A
	* suspended one is taken out of the waiting lists and destroyed at
	* once; one that is executing cannot be destroyed under its own feet,
	* so it is only marked and destroyed at its next co_await.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entity のいずれかのコンポーネントに属する全コルーチンをキャンセルする。
	* 中断中のものは待機一覧から外してすぐに破棄する。実行中のものは実行
	* している最中に破棄することはできないので、印だけ付けて次の co_await で
	* 破棄する。
	*/
	void CoroutineSystem::Cancel(EntityID entity)
	{
		/// [EN] Collect first: destroying a frame unregisters it from live_, which must not happen while live_ is being walked.
		/// [JP] 先に集める。フレームを破棄すると live_ から登録解除されるので、live_ を回している最中に破棄してはならない。
		DynamicArray<std::coroutine_handle<Coroutine::promise_type>> targets;
		for (std::coroutine_handle<Coroutine::promise_type> handle : live_)
		{
			const Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			if (manage.entityID_ != entity)
			{
				continue;
			}

			/// [EN] Already marked; it will be destroyed at its next co_await.
			/// [JP] 既に印が付いている。次の co_await で破棄される。
			if (manage.cancelled_)
			{
				continue;
			}

			targets.push_back(handle);
		}

		for (std::coroutine_handle<Coroutine::promise_type> handle : targets)
		{
			Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			/// [EN] Executing right now (e.g. it destroyed its own actor): only mark it, ScheduleSeconds/ScheduleFrames destroy it at the next co_await.
			/// [JP] 今まさに実行中（例: 自分の actor を破棄した）: 印だけ付け、次の co_await で ScheduleSeconds/ScheduleFrames が破棄する。
			if (!manage.suspended_)
			{
				manage.cancelled_ = true;
				continue;
			}

			/// [EN] Suspended: it sits in exactly one of the waiting lists or in this frame's batch; take it out of all of them so nothing resumes it later.
			/// [JP] 中断中: 待機一覧のいずれか1つか、このフレームの一群のどこかにいる。後で誰も再開しないように、全てから外す。
			erase_if(timer_, [handle](const TimeEntry& entry) { return entry.handle_ == handle; });
			erase_if(frame_, [handle](const FrameEntry& entry) { return entry.handle_ == handle; });
			erase_if(ready_, [handle](std::coroutine_handle<Coroutine::promise_type> entry) { return entry == handle; });

			/// [EN] The batch is cleared in place rather than erased, since Update may be walking it by index right now.
			/// [JP] Update が今まさに添字で回しているかもしれないので、一群からは削除せずその場で空にする。
			for (std::coroutine_handle<Coroutine::promise_type>& entry : resuming_)
			{
				if (entry == handle)
				{
					entry = nullptr;
				}
			}

			/// [EN] Destroyed while its component still exists, so the destructors of the body's locals never see a freed component. The promise destructor unregisters it from live_.
			/// [JP] コンポーネントがまだ存在するうちに破棄するので、本体のローカル変数のデストラクタが解放済みのコンポーネントを見ることは無い。live_ からの登録解除は promise のデストラクタが行う。
			handle.destroy();
		}
	}

	/**
	* [EN]
	* Cancels every coroutine belonging to entity's component of type
	* component. A suspended one is taken out of the waiting lists and
	* destroyed at once; one that is executing cannot be destroyed under
	* its own feet, so it is only marked and destroyed at its next co_await.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entity の、型が component のコンポーネントに属する全コルーチンを
	* キャンセルする。中断中のものは待機一覧から外してすぐに破棄する。
	* 実行中のものは実行している最中に破棄することはできないので、印だけ
	* 付けて次の co_await で破棄する。
	*/
	void CoroutineSystem::Cancel(EntityID entity, ComponentID component)
	{
		/// [EN] Collect first: destroying a frame unregisters it from live_, which must not happen while live_ is being walked.
		/// [JP] 先に集める。フレームを破棄すると live_ から登録解除されるので、live_ を回している最中に破棄してはならない。
		DynamicArray<std::coroutine_handle<Coroutine::promise_type>> targets;
		for (std::coroutine_handle<Coroutine::promise_type> handle : live_)
		{
			const Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			if (manage.entityID_ != entity || manage.componentID_ != component)
			{
				continue;
			}

			/// [EN] Already marked; it will be destroyed at its next co_await.
			/// [JP] 既に印が付いている。次の co_await で破棄される。
			if (manage.cancelled_)
			{
				continue;
			}

			targets.push_back(handle);
		}

		for (std::coroutine_handle<Coroutine::promise_type> handle : targets)
		{
			Coroutine::promise_type::Manage& manage = handle.promise().manage_;

			/// [EN] Executing right now (e.g. it removed its own component): only mark it, ScheduleSeconds/ScheduleFrames destroy it at the next co_await.
			/// [JP] 今まさに実行中（例: 自分のコンポーネントを削除した）: 印だけ付け、次の co_await で ScheduleSeconds/ScheduleFrames が破棄する。
			if (!manage.suspended_)
			{
				manage.cancelled_ = true;
				continue;
			}

			/// [EN] Suspended: it sits in exactly one of the waiting lists or in this frame's batch; take it out of all of them so nothing resumes it later.
			/// [JP] 中断中: 待機一覧のいずれか1つか、このフレームの一群のどこかにいる。後で誰も再開しないように、全てから外す。
			erase_if(timer_, [handle](const TimeEntry& entry) { return entry.handle_ == handle; });
			erase_if(frame_, [handle](const FrameEntry& entry) { return entry.handle_ == handle; });
			erase_if(ready_, [handle](std::coroutine_handle<Coroutine::promise_type> entry) { return entry == handle; });

			/// [EN] The batch is cleared in place rather than erased, since Update may be walking it by index right now.
			/// [JP] Update が今まさに添字で回しているかもしれないので、一群からは削除せずその場で空にする。
			for (std::coroutine_handle<Coroutine::promise_type>& entry : resuming_)
			{
				if (entry == handle)
				{
					entry = nullptr;
				}
			}

			/// [EN] Destroyed while its component still exists, so the destructors of the body's locals never see a freed component. The promise destructor unregisters it from live_.
			/// [JP] コンポーネントがまだ存在するうちに破棄するので、本体のローカル変数のデストラクタが解放済みのコンポーネントを見ることは無い。live_ からの登録解除は promise のデストラクタが行う。
			handle.destroy();
		}
	}

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
	void CoroutineSystem::ScheduleSeconds(std::coroutine_handle<Coroutine::promise_type> handle, Float seconds)
	{
		Coroutine::promise_type::Manage& manage = handle.promise().manage_;

		/// [EN] Called from await_suspend, where the coroutine is already fully suspended, so destroying it here is safe as long as the awaiter is not touched afterwards (the awaiters only return).
		/// [JP] await_suspend から呼ばれ、その時点でコルーチンは完全に中断しているので、以後 awaiter に触れなければここで破棄しても安全（awaiter は戻るだけ）。
		if (manage.cancelled_)
		{
			handle.destroy();
			return;
		}

		/// [EN] From here on Cancel treats it as waiting, so it is destroyed at once instead of being marked.
		/// [JP] ここから先、Cancel はこれを待機中として扱い、印を付けるのではなくすぐに破棄する。
		manage.suspended_ = true;
		timer_.push_back({ handle, seconds });
	}

	/**
	* [EN]
	* Suspends handle for frames played frames. A coroutine that was
	* cancelled while executing is destroyed here instead.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* handle をプレイ中の frames フレーム中断させる。実行中にキャンセル
	* されたコルーチンは、代わりにここで破棄する。
	*/
	void CoroutineSystem::ScheduleFrames(std::coroutine_handle<Coroutine::promise_type> handle, Int frames)
	{
		Coroutine::promise_type::Manage& manage = handle.promise().manage_;

		/// [EN] Called from await_suspend, where the coroutine is already fully suspended, so destroying it here is safe as long as the awaiter is not touched afterwards (the awaiters only return).
		/// [JP] await_suspend から呼ばれ、その時点でコルーチンは完全に中断しているので、以後 awaiter に触れなければここで破棄しても安全（awaiter は戻るだけ）。
		if (manage.cancelled_)
		{
			handle.destroy();
			return;
		}

		/// [EN] From here on Cancel treats it as waiting, so it is destroyed at once instead of being marked.
		/// [JP] ここから先、Cancel はこれを待機中として扱い、印を付けるのではなくすぐに破棄する。
		manage.suspended_ = true;
		frame_.push_back({ handle, frames });
	}

	/**
	* [EN]
	* Adds a coroutine that has just started to the live list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 開始したばかりのコルーチンを生存一覧に加える。
	*/
	void CoroutineSystem::Register(std::coroutine_handle<Coroutine::promise_type> handle)
	{
		live_.push_back(handle);
	}

	/**
	* [EN]
	* Removes a coroutine whose frame is being freed from the live list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームが解放されようとしているコルーチンを生存一覧から取り除く。
	*/
	void CoroutineSystem::Unregister(std::coroutine_handle<Coroutine::promise_type> handle)
	{
		auto it = std::find(live_.begin(), live_.end(), handle);
		if (it == live_.end())
		{
			return;
		}

		/// [EN] The order of live_ carries no meaning, so the last entry simply fills the gap.
		/// [JP] live_ の並び順に意味は無いので、末尾のエントリで穴を埋めるだけでよい。
		*it = live_.back();
		live_.pop_back();
	}
}
