#pragma once
#include <GraphicsEngine/Environment/Weather/Weather.h>

namespace SeedCore
{
	class World;

	struct VolumetricCloudScapes;

	/**
	* [EN]
	* Run by GraphicsUpdater once per frame after CelestialSystem: finds the scene's Weather component (singleton-by-convention, see
	* Environment/Weather/Weather.h) and interpolates the scene's VolumetricCloudScapes
	* component toward the current WeatherType's target values. Also advances the
	* seasonal auto-cycle (monthOfYear-weighted, force flags bypass the
	* seasonal weighting), the wetness_/snowCoverage_ ramps, and thunder
	* strike/flash timing. A no-op when the scene has no Weather component, so
	* manual cloud tuning in the editor is left alone.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GraphicsUpdater が毎フレーム CelestialSystem の後に回す。シーンの Weather コンポーネント(singleton的運用、Environment/Weather/Weather.h
	* 参照)を見つけ、シーンの VolumetricCloudScapes コンポーネントを現在の
	* WeatherType の目標値へ補間する。季節による自動サイクル(monthOfYear で重み付け、
	* forceフラグは季節の重みを無視)、wetness_/snowCoverage_ の増減、
	* 雷の発生/閃光タイミングも進める。シーンに Weather コンポーネントが
	* 無ければ何もしないので、エディタでの手動雲チューニングには干渉しない。
	*/
	class SEEDCORE_API WeatherSystem
	{
	public:
		void Execute(World& world, Float deltaTime, Uint32 monthOfYear);

	private:
		static void GetTarget(WeatherType type, VolumetricCloudScapes& target);

		NODISCARD WeatherType PickRandomType(Uint32 monthOfYear, const Weather& weather);

		NODISCARD Float RandomRange(Float minValue, Float maxValue);

		std::mt19937 randomEngine_ = std::mt19937(std::random_device{}());
	};
}
