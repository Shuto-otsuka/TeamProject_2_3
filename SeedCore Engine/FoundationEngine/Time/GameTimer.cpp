#include <FoundationEngine/Time/GameTimer.h>

namespace SeedCore
{
	void GameTimer::Play()
	{
		if (!playing_)
		{
			playing_ = true;
			paused_ = false;
		}
	}

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

	void GameTimer::Pause()
	{
		if (playing_ && !paused_)
		{
			paused_ = true;
		}
	}

	void GameTimer::Resume()
	{
		if (playing_ && paused_)
		{
			paused_ = false;
		}
	}

	void GameTimer::Tick(Float worldDelta)
	{
		if (!playing_ || paused_)
		{
			scaledDelta_ = 0.0f;
			unscaledDelta_ = 0.0f;
			return;
		}

		unscaledDelta_ = worldDelta;
		scaledDelta_ = worldDelta * timeScale_;

		unscaledTotal_ += static_cast<Double>(unscaledDelta_);
		scaledTotal_ += static_cast<Double>(scaledDelta_);

		constexpr Float maxAccumulatedSteps = 8.0f;
		accumulator_ = Min(accumulator_ + scaledDelta_, fixedDeltaTime_ * maxAccumulatedSteps);
	}

	Float GameTimer::ScaledDeltaTime()const
	{
		return scaledDelta_;
	}

	Float GameTimer::UnscaledDeltaTime()const
	{
		return unscaledDelta_;
	}

	Float GameTimer::ScaledTotalTime()const
	{
		return static_cast<Float>(scaledTotal_);
	}

	Float GameTimer::UnscaledTotalTime()const
	{
		return static_cast<Float>(unscaledTotal_);
	}

	Float GameTimer::FixedDeltaTime()const
	{
		return fixedDeltaTime_;
	}

	Bool GameTimer::Step()
	{
		if (accumulator_ < fixedDeltaTime_)
		{
			return false;
		}

		accumulator_ -= fixedDeltaTime_;
		return true;
	}

	Float GameTimer::TimeScale()const
	{
		return timeScale_;
	}

	void GameTimer::TimeScale(Float scale)
	{
		timeScale_ = scale;
	}

	Bool GameTimer::Playing()const
	{
		return playing_;
	}

	Bool GameTimer::Paused()const
	{
		return paused_;
	}
}
