#include <FoundationEngine/Time/Timer.h>

namespace SeedCore
{
	/**
	* [EN]
	* (Re)starts the timer, adding any time spent stopped to paused_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイマーを（再）開始する。停止していた時間があれば paused_ に加算する。
	*/
	void Timer::Start()
	{
		/// [EN] Resuming from Stop(): the stretch from stop_ to now is added to paused_, so Total() leaves it out.
		/// [JP] Stop() からの再開: stop_ から今までの区間を paused_ に足し、Total() がその分を含めないようにする。
		if (stopped_)
		{
			paused_ += std::chrono::duration<Double>(std::chrono::high_resolution_clock::now() - stop_);
			stopped_ = false;
		}

		/// [EN] The next Tick() measures from now, so the first delta after a restart never includes the stopped time.
		/// [JP] 次の Tick() はここから測るので、再開直後のデルタに停止していた時間が入ることは無い。
		last_ = std::chrono::high_resolution_clock::now();
	}

	/**
	* [EN]
	* Stops the timer; elapsed time while stopped is excluded from Total().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイマーを停止する。停止中の経過時間は Total() から除外される。
	*/
	void Timer::Stop()
	{
		/// [EN] Only the first Stop() records the time; stopping again keeps the original stop_, so the whole stopped stretch is counted once.
		/// [JP] 時刻を記録するのは最初の Stop() だけ。重ねて止めても元の stop_ を保つので、停止区間全体が一度だけ数えられる。
		if (!stopped_)
		{
			stop_ = std::chrono::high_resolution_clock::now();
			stopped_ = true;
		}
	}

	/**
	* [EN]
	* Resets the timer to its initial state (start time, delta, pause time).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイマーを初期状態（開始時刻・デルタ・停止時間）にリセットする。
	*/
	void Timer::Reset()
	{
		/// [EN] Both Total() and the next Tick() measure from now.
		/// [JP] Total() も次の Tick() も、ここから測り始める。
		start_ = std::chrono::high_resolution_clock::now();
		last_ = std::chrono::high_resolution_clock::now();

		/// [EN] Stopped time accumulated before the reset no longer applies to the new start_.
		/// [JP] リセット前に貯まった停止時間は、新しい start_ には関係ないので捨てる。
		paused_ = std::chrono::duration<Double>::zero();

		/// [EN] The timer comes out running, with no delta until the next Tick().
		/// [JP] リセット後のタイマーは動いている状態で、次の Tick() まではデルタ 0。
		delta_ = 0.0f;
		stopped_ = false;
	}

	/**
	* [EN]
	* Advances one tick, updating delta_ from the time since the
	* last Tick()/Start(). No-op (delta becomes 0) while stopped.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1ティック進め、前回の Tick()/Start() からの経過時間で
	* delta_ を更新する。停止中は何もしない（デルタは0になる）。
	*/
	void Timer::Tick()
	{
		/// [EN] While stopped no time passes; last_ is left alone because Start() resets it on resume.
		/// [JP] 停止中は時間が進まない。再開時に Start() が last_ を置き直すので、ここでは触らない。
		if (stopped_)
		{
			delta_ = 0.0f;
			return;
		}

		/// [EN] Delta is the time since the previous Tick()/Start(); last_ then moves to now for the next one.
		/// [JP] デルタは前回の Tick()/Start() からの時間。その後 last_ を今に進め、次の計測の起点にする。
		delta_ = std::chrono::duration<Double>(std::chrono::high_resolution_clock::now() - last_).count();
		last_ = std::chrono::high_resolution_clock::now();
	}

	/**
	* [EN]
	* Returns the delta time computed by the last Tick(), in seconds.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の Tick() で計算されたデルタタイムを秒単位で返す。
	*/
	Float Timer::Delta()const
	{
		/// [EN] Kept in Double internally for precision; narrowed to Float only when handed out.
		/// [JP] 内部では精度のため Double で持ち、渡すときだけ Float に落とす。
		return static_cast<Float>(delta_);
	}

	/**
	* [EN]
	* Returns the total elapsed time since Reset(), excluding stopped time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Reset() からの合計経過時間を、停止時間を除いて秒単位で返す。
	*/
	Float Timer::Total()const
	{
		/// [EN] While stopped the total freezes at stop_; while running it advances with each Tick() (last_), not with the live clock.
		/// [JP] 停止中は合計が stop_ の時点で止まる。動いている間は現在時刻ではなく Tick() ごと（last_）に進む。
		auto end = stopped_ ? stop_ : last_;

		/// [EN] Elapsed since start_, minus the stretches spent stopped that Start() has folded into paused_.
		/// [JP] start_ からの経過時間から、Start() が paused_ にまとめた停止区間を差し引く。
		return static_cast<Float>(std::chrono::duration<Double>(end - start_ - paused_).count());
	}
}
