#pragma once

namespace SeedCore
{
	class SEEDCORE_API GameTimer
	{
	public:
		void Play();

		void Stop();

		void Pause();

		void Resume();

		void Tick(Float worldDelta);

		Float ScaledDeltaTime()const;

		Float UnscaledDeltaTime()const;

		Float ScaledTotalTime()const;

		Float UnscaledTotalTime()const;

		Float FixedDeltaTime()const;

		Bool Step();

		Float TimeScale()const;

		void TimeScale(Float scale);

		Bool Playing()const;

		Bool Paused()const;

	private:
		Float scaledDelta_ = 0.0f;

		Float unscaledDelta_ = 0.0f;

		Double scaledTotal_ = 0.0;

		Double unscaledTotal_ = 0.0;

		Float fixedDeltaTime_ = 1.0f / 60.0f;

		Float accumulator_ = 0.0f;

		Float timeScale_ = 1.0f;

		Bool playing_ = false;

		Bool paused_ = false;
	};
}
