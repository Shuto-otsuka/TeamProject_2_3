#include <FoundationEngine/Time/GameTimer.h>

namespace SeedCore
{
	/**
	* [EN]
	* Starts playing (or resumes from a paused state via Pause/Resume).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 再生を開始する（Pause/Resume による一時停止からの復帰は含まない）。
	*/
	void GameTimer::Play()
	{
		if (!playing_)
		{
			playing_ = true;
			paused_ = false;
		}
	}

	/**
	* [EN]
	* Stops playback entirely, resetting both total-time accumulators, the
	* fixed-step accumulator, and the time scale back to 1.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 再生を完全に停止し、2つの合計時間の累積値、固定ステップの蓄積時間、
	* タイムスケール（1 に戻す）をリセットする。
	*/
	void GameTimer::Stop()
	{
		if (playing_ || paused_)
		{
			playing_ = false;
			paused_ = false;
			scaledTotal_ = 0.0;
			unscaledTotal_ = 0.0;
			accumulator_ = 0.0f;
			timeScale_ = 1.0f;
		}
	}

	/**
	* [EN]
	* Pauses playback; Tick() becomes a no-op until Resume().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 再生を一時停止する。Resume() されるまで Tick() は何もしない。
	*/
	void GameTimer::Pause()
	{
		if (playing_ && !paused_)
		{
			paused_ = true;
		}
	}

	/**
	* [EN]
	* Resumes playback after Pause().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Pause() の後、再生を再開する。
	*/
	void GameTimer::Resume()
	{
		if (playing_ && paused_)
		{
			paused_ = false;
		}
	}

	/**
	* [EN]
	* Advances one tick using worldDelta (the frame's real delta time).
	* Updates both the time-scaled and unscaled delta/total, and feeds
	* the scaled delta into the fixed-step accumulator. While stopped or
	* paused, both deltas become 0 and nothing accumulates.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* worldDelta（フレームの実時間デルタ）を用いて1ティック進める。
	* タイムスケール適用後と未適用の両方のデルタ/合計を更新し、適用後の
	* デルタを固定ステップの蓄積時間に足す。停止中または一時停止中は
	* 両方のデルタが 0 になり、何も蓄積しない。
	*/
	void GameTimer::Tick(Float worldDelta)
	{
		if (!playing_ || paused_)
		{
			scaledDelta_ = 0.0f;
			unscaledDelta_ = 0.0f;
			return;
		}

		/// [EN] Derive both the unscaled and time-scaled delta from worldDelta before accumulating either total.
		/// [JP] どちらの合計を累積する前にも、worldDelta からタイムスケール未適用/適用後の両方のデルタを導出する。
		unscaledDelta_ = worldDelta;
		scaledDelta_ = worldDelta * timeScale_;

		unscaledTotal_ += static_cast<Double>(unscaledDelta_);
		scaledTotal_ += static_cast<Double>(scaledDelta_);

		/// [EN] The accumulator is capped at a few steps' worth, so a long stall costs at most that many catch-up steps.
		/// [JP] 蓄積時間は数ステップ分で頭打ちにし、長い停止の後でも追いつくためのステップはその回数までで済ませる。
		constexpr Float maxAccumulatedSteps = 8.0f;
		accumulator_ = Min(accumulator_ + scaledDelta_, fixedDeltaTime_ * maxAccumulatedSteps);
	}

	/**
	* [EN]
	* Returns the time-scaled delta time from the last Tick().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の Tick() によるタイムスケール適用後のデルタタイムを返す。
	*/
	Float GameTimer::ScaledDeltaTime()const
	{
		return scaledDelta_;
	}

	/**
	* [EN]
	* Returns the unscaled delta time from the last Tick().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の Tick() によるタイムスケール未適用のデルタタイムを返す。
	*/
	Float GameTimer::UnscaledDeltaTime()const
	{
		return unscaledDelta_;
	}

	/**
	* [EN]
	* Returns the time-scaled total time accumulated since Play()/Stop().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Play()/Stop() 以降に累積された、タイムスケール適用後の合計時間を返す。
	*/
	Float GameTimer::ScaledTotalTime()const
	{
		return static_cast<Float>(scaledTotal_);
	}

	/**
	* [EN]
	* Returns the unscaled total time accumulated since Play()/Stop().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Play()/Stop() 以降に累積された、タイムスケール未適用の合計時間を返す。
	*/
	Float GameTimer::UnscaledTotalTime()const
	{
		return static_cast<Float>(unscaledTotal_);
	}

	/**
	* [EN]
	* Returns the fixed timestep used for physics/FixedTick stepping.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理/FixedTick のステップに使う固定タイムステップを返す。
	*/
	Float GameTimer::FixedDeltaTime()const
	{
		return fixedDeltaTime_;
	}

	/**
	* [EN]
	* Consumes one FixedDeltaTime() worth of accumulated scaled time, if
	* enough has built up since the last successful call.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の成功呼び出し以降に十分な時間が蓄積されていれば、タイムスケール
	* 適用後の蓄積時間を FixedDeltaTime() 分だけ1回消費する。
	*/
	Bool GameTimer::Step()
	{
		if (accumulator_ < fixedDeltaTime_)
		{
			return false;
		}

		accumulator_ -= fixedDeltaTime_;
		return true;
	}

	/**
	* [EN]
	* Returns the current time scale multiplier.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在のタイムスケール倍率を返す。
	*/
	Float GameTimer::TimeScale()const
	{
		return timeScale_;
	}

	/**
	* [EN]
	* Sets the time scale multiplier applied to worldDelta in Tick().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Tick() で worldDelta に適用されるタイムスケール倍率を設定する。
	*/
	void GameTimer::TimeScale(Float scale)
	{
		timeScale_ = scale;
	}

	/**
	* [EN]
	* Returns whether the timer is currently playing (regardless of pause state).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイマーが現在再生中かどうかを返す（一時停止状態は問わない）。
	*/
	Bool GameTimer::Playing()const
	{
		return playing_;
	}

	/**
	* [EN]
	* Returns whether the timer is currently paused.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイマーが現在一時停止中かどうかを返す。
	*/
	Bool GameTimer::Paused()const
	{
		return paused_;
	}
}
