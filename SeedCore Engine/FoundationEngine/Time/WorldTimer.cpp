#include <FoundationEngine/Time/WorldTimer.h>

namespace SeedCore
{
	void WorldTimer::Tick(Float delta)
	{
		delta_ = delta;
		total_ += static_cast<Double>(delta);
	}

	Float WorldTimer::DeltaTime()const
	{
		return delta_;
	}

	Float WorldTimer::TotalTime()const
	{
		return static_cast<Float>(total_);
	}
}
