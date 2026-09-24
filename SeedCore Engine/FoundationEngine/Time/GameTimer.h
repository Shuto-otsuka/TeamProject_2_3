#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Gameplay clock driven by the frame's real delta, with play/pause/stop
	* state and an adjustable time scale (slow motion, fast forward, etc.).
	* It also owns the fixed-step accumulator, so Tick, FixedTick and
	* physics all follow the same pause state and time scale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの実時間デルタで進む、ゲームプレイ用クロック。
	* 再生/一時停止/停止の状態と、調整可能なタイムスケール（スローモーション、
	* 早送りなど）を持つ。固定ステップの蓄積もこのクラスが持つので、Tick・
	* FixedTick・物理はすべて同じ一時停止状態とタイムスケールに従う。
	*/
	class SEEDCORE_API GameTimer
	{
	public:
		/**
		* [EN]
		* Starts playing (or resumes from a paused state via Pause/Resume).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 再生を開始する（Pause/Resume による一時停止からの復帰は含まない）。
		*/
		void Play();

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
		void Stop();

		/**
		* [EN]
		* Pauses playback; Tick() becomes a no-op until Resume().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 再生を一時停止する。Resume() されるまで Tick() は何もしない。
		*/
		void Pause();

		/**
		* [EN]
		* Resumes playback after Pause().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Pause() の後、再生を再開する。
		*/
		void Resume();

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
		void Tick(Float worldDelta);

		/**
		* [EN]
		* Returns the time-scaled delta time from the last Tick().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近の Tick() によるタイムスケール適用後のデルタタイムを返す。
		*/
		Float ScaledDeltaTime()const;

		/**
		* [EN]
		* Returns the unscaled delta time from the last Tick().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近の Tick() によるタイムスケール未適用のデルタタイムを返す。
		*/
		Float UnscaledDeltaTime()const;

		/**
		* [EN]
		* Returns the time-scaled total time accumulated since Play()/Stop().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Play()/Stop() 以降に累積された、タイムスケール適用後の合計時間を返す。
		*/
		Float ScaledTotalTime()const;

		/**
		* [EN]
		* Returns the unscaled total time accumulated since Play()/Stop().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Play()/Stop() 以降に累積された、タイムスケール未適用の合計時間を返す。
		*/
		Float UnscaledTotalTime()const;

		/**
		* [EN]
		* Returns the fixed timestep used for physics/FixedTick stepping.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理/FixedTick のステップに使う固定タイムステップを返す。
		*/
		Float FixedDeltaTime()const;

		/**
		* [EN]
		* Consumes one FixedDeltaTime() worth of accumulated scaled time, if
		* enough has built up since the last successful call. Intended to be
		* called in a loop after Tick() so the caller advances physics/
		* FixedTick exactly as many times as the accumulated game time
		* covers, regardless of the rendered frame rate. Because only scaled
		* time is accumulated, pausing stops the steps and the time scale
		* changes how often they run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の成功呼び出し以降に十分な時間が蓄積されていれば、タイムスケール
		* 適用後の蓄積時間を FixedDeltaTime() 分だけ1回消費する。Tick() の後に
		* ループで呼ぶことを想定しており、描画フレームレートに関係なく、蓄積された
		* ゲーム時間が賄う回数だけ呼び出し側が物理/FixedTick を進められるように
		* する。蓄積するのはタイムスケール適用後の時間なので、一時停止中は
		* ステップが止まり、タイムスケールによってステップの回数が変わる。
		*/
		Bool Step();

		/**
		* [EN]
		* Returns the current time scale multiplier.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のタイムスケール倍率を返す。
		*/
		Float TimeScale()const;

		/**
		* [EN]
		* Sets the time scale multiplier applied to worldDelta in Tick().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Tick() で worldDelta に適用されるタイムスケール倍率を設定する。
		*/
		void TimeScale(Float scale);

		/**
		* [EN]
		* Returns whether the timer is currently playing (regardless of pause state).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タイマーが現在再生中かどうかを返す（一時停止状態は問わない）。
		*/
		Bool Playing()const;

		/**
		* [EN]
		* Returns whether the timer is currently paused.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タイマーが現在一時停止中かどうかを返す。
		*/
		Bool Paused()const;

	private:
		/// [EN] Time-scaled delta time from the last Tick().
		/// [JP] 直近の Tick() によるタイムスケール適用後のデルタタイム。
		Float scaledDelta_ = 0.0f;

		/// [EN] Unscaled delta time from the last Tick().
		/// [JP] 直近の Tick() によるタイムスケール未適用のデルタタイム。
		Float unscaledDelta_ = 0.0f;

		/// [EN] Time-scaled total time accumulated since Play()/Stop().
		/// [JP] Play()/Stop() 以降に累積された、タイムスケール適用後の合計時間。
		Double scaledTotal_ = 0.0;

		/// [EN] Unscaled total time accumulated since Play()/Stop().
		/// [JP] Play()/Stop() 以降に累積された、タイムスケール未適用の合計時間。
		Double unscaledTotal_ = 0.0;

		/// [EN] Fixed timestep used for physics/FixedTick stepping.
		/// [JP] 物理/FixedTick のステップに使う固定タイムステップ。
		Float fixedDeltaTime_ = 1.0f / 60.0f;

		/// [EN] Scaled time accumulated since the last successful Step(), consumed one FixedDeltaTime() at a time.
		/// [JP] 直近の Step() 成功以降に蓄積された、タイムスケール適用後の時間。FixedDeltaTime() ずつ消費される。
		Float accumulator_ = 0.0f;

		/// [EN] Multiplier applied to worldDelta in Tick().
		/// [JP] Tick() で worldDelta に適用される倍率。
		Float timeScale_ = 1.0f;

		/// [EN] Whether the timer is currently playing.
		/// [JP] タイマーが現在再生中かどうか。
		Bool playing_ = false;

		/// [EN] Whether the timer is currently paused.
		/// [JP] タイマーが現在一時停止中かどうか。
		Bool paused_ = false;
	};
}
