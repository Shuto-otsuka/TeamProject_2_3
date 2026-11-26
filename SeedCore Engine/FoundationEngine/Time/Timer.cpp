#include <FoundationEngine/Time/Timer.h>

namespace SeedCore
{
	void Timer::Start()
	{
		if (stopped_)
		{
			paused_ += std::chrono::duration<Double>(std::chrono::high_resolution_clock::now() - stop_);
			stopped_ = false;
		}

		last_ = std::chrono::high_resolution_clock::now();
	}

	void Timer::Stop()
	{
		if (!stopped_)
		{
			stop_ = std::chrono::high_resolution_clock::now();
			stopped_ = true;
		}
	}

	void Timer::Reset()
	{
		start_ = std::chrono::high_resolution_clock::now();
		last_ = std::chrono::high_resolution_clock::now();

		paused_ = std::chrono::duration<Double>::zero();

		delta_ = 0.0f;
		stopped_ = false;
	}

	void Timer::Tick()
	{
		if (stopped_)
		{
			delta_ = 0.0f;
			return;
		}

		delta_ = std::chrono::duration<Double>(std::chrono::high_resolution_clock::now() - last_).count();
		last_ = std::chrono::high_resolution_clock::now();
	}

	Float Timer::Delta()const
	{
		return static_cast<Float>(delta_);
	}

	Float Timer::Total()const
	{
		auto end = stopped_ ? stop_ : last_;

		return static_cast<Float>(std::chrono::duration<Double>(end - start_ - paused_).count());
	}
}
