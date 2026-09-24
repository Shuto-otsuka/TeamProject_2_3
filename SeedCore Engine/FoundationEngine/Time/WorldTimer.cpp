#include <FoundationEngine/Time/WorldTimer.h>

namespace SeedCore
{
	/**
	* [EN]
	* Advances one frame by delta seconds, accumulating into total_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* delta 秒ぶん1フレーム進め、total_ に累積する。
	*/
	void WorldTimer::Tick(Float delta)
	{
		delta_ = delta;
		total_ += static_cast<Double>(delta);
	}

	/**
	* [EN]
	* Returns the delta time passed to the last Tick().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の Tick() に渡されたデルタタイムを返す。
	*/
	Float WorldTimer::DeltaTime()const
	{
		return delta_;
	}

	/**
	* [EN]
	* Returns the total time accumulated since engine start.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンジン起動からの累積合計時間を返す。
	*/
	Float WorldTimer::TotalTime()const
	{
		return static_cast<Float>(total_);
	}
}
