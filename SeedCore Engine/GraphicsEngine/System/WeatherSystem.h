#pragma once
#include <GraphicsEngine/Environment/Weather/Weather.h>

namespace SeedCore
{
	class World;

	struct VolumetricCloudScapes;

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
