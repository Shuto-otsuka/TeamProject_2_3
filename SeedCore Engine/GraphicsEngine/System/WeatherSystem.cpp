#include <GraphicsEngine/System/WeatherSystem.h>
#include <GraphicsEngine/Environment/Weather/Weather.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	namespace
	{
		Float Lerp(Float from, Float to, Float t)
		{
			return from + (to - from) * t;
		}

		Float RainSeasonalWeight(Uint32 monthOfYear)
		{
			if (monthOfYear == 5 || monthOfYear == 6 || monthOfYear == 7)
			{
				return 2.5f;
			}
			return 1.0f;
		}

		Bool SnowSeason(Uint32 monthOfYear)
		{
			return monthOfYear == 12 || monthOfYear == 1 || monthOfYear == 2;
		}
	}

	void WeatherSystem::GetTarget(WeatherType type, VolumetricCloudScapes& target)
	{
		switch (type)
		{
		case WeatherType::Clear:
			target.coverage_ = 0.15f;
			target.lighting_.density_ = 0.5f;
			target.rain_ = 0.0f;
			target.windSpeed_ = 5.0f;
			target.sky_.brightness_ = 1.0f;
			break;

		case WeatherType::Cloudy:
			target.coverage_ = 0.45f;
			target.lighting_.density_ = 0.5f;
			target.rain_ = 0.0f;
			target.windSpeed_ = 10.0f;
			target.sky_.brightness_ = 0.9f;
			break;

		case WeatherType::Overcast:
			target.coverage_ = 0.85f;
			target.lighting_.density_ = 0.6f;
			target.rain_ = 0.0f;
			target.windSpeed_ = 15.0f;
			target.sky_.brightness_ = 0.6f;
			break;

		case WeatherType::Rain:
			target.coverage_ = 0.9f;
			target.lighting_.density_ = 0.62f;
			target.rain_ = 0.7f;
			target.windSpeed_ = 25.0f;
			target.sky_.brightness_ = 0.45f;
			break;

		case WeatherType::Snow:
			target.coverage_ = 0.8f;
			target.lighting_.density_ = 0.6f;
			target.rain_ = 0.0f;
			target.windSpeed_ = 10.0f;
			target.sky_.brightness_ = 0.55f;
			break;

		case WeatherType::Storm:
			target.coverage_ = 1.0f;
			target.lighting_.density_ = 0.68f;
			target.rain_ = 1.0f;
			target.windSpeed_ = 45.0f;
			target.sky_.brightness_ = 0.3f;
			break;
		}
	}

	Float WeatherSystem::RandomRange(Float minValue, Float maxValue)
	{
		std::uniform_real_distribution<Float> distribution(minValue, maxValue);
		return distribution(randomEngine_);
	}

	WeatherType WeatherSystem::PickRandomType(Uint32 monthOfYear, const Weather& weather)
	{
		struct Entry
		{
			WeatherType type_;
			Float weight_;
		};

		Entry entries[] =
		{
			{ WeatherType::Clear, 3.0f },
			{ WeatherType::Cloudy, 2.0f },
			{ WeatherType::Overcast, 1.0f },
			{ WeatherType::Rain, weather.forceRain_ ? 2.0f : RainSeasonalWeight(monthOfYear) },
			{ WeatherType::Snow, weather.forceSnow_ ? 1.5f : (SnowSeason(monthOfYear) ? 1.5f : 0.0f) },
			{ WeatherType::Storm, weather.forceStorm_ ? 1.0f : 0.15f },
		};

		Float totalWeight = 0.0f;
		for (const Entry& entry : entries)
		{
			totalWeight += entry.weight_;
		}

		Float roll = RandomRange(0.0f, Max(totalWeight, 0.0001f));
		Float cumulative = 0.0f;
		for (const Entry& entry : entries)
		{
			cumulative += entry.weight_;
			if (roll <= cumulative)
			{
				return entry.type_;
			}
		}

		return WeatherType::Clear;
	}

	void WeatherSystem::Execute(World& world, Float deltaTime, Uint32 monthOfYear)
	{
		Query<Read<Active>, Write<Weather>> query(world);

		Bool found = false;
		WeatherType weatherType = WeatherType::Clear;
		Float t = 0.0f;

		query.ForEach([&](const Active& active, Weather& weather)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;

				if (weather.autoCycle_)
				{
					weather.autoCycleTimer_ -= deltaTime;
					if (weather.autoCycleTimer_ <= 0.0f)
					{
						std::uniform_real_distribution<Float> intervalDistribution(weather.minIntervalMinutes_, weather.maxIntervalMinutes_);
						weather.autoCycleTimer_ = intervalDistribution(randomEngine_) * 60.0f;

						weather.type_ = PickRandomType(monthOfYear, weather);
					}
				}

				weatherType = weather.type_;
				t = weather.transitionSeconds_ > 0.0f ? std::clamp(deltaTime / weather.transitionSeconds_, 0.0f, 1.0f) : 1.0f;

				Bool raining = weather.type_ == WeatherType::Rain || weather.type_ == WeatherType::Storm;
				Float wetnessRampSpeed = 1.0f / 20.0f;
				Float wetnessDecaySpeed = 1.0f / 90.0f;
				weather.wetness_ += deltaTime * (raining ? wetnessRampSpeed : -wetnessDecaySpeed);
				weather.wetness_ = std::clamp(weather.wetness_, 0.0f, 1.0f);

				Bool snowing = weather.type_ == WeatherType::Snow;
				Float snowRampSpeed = 1.0f / 60.0f;
				Float snowMeltSpeed = SnowSeason(monthOfYear) ? (1.0f / 180.0f) : (1.0f / 40.0f);
				weather.snowCoverage_ += deltaTime * (snowing ? snowRampSpeed : -snowMeltSpeed);
				weather.snowCoverage_ = std::clamp(weather.snowCoverage_, 0.0f, 1.0f);

				const Float snowIntensityRate = 1.0f / 2.0f;
				weather.snowIntensity_ += deltaTime * (snowing ? snowIntensityRate : -snowIntensityRate);
				weather.snowIntensity_ = std::clamp(weather.snowIntensity_, 0.0f, 1.0f);

				Float thunderChancePerSecond = 0.0f;
				if (weather.type_ == WeatherType::Storm)
				{
					thunderChancePerSecond = 0.005f;
				}

				if (weather.forceThunder_)
				{
					thunderChancePerSecond = Max(thunderChancePerSecond, 0.05f);
				}

				if (thunderChancePerSecond > 0.0f && RandomRange(0.0f, 1.0f) < thunderChancePerSecond * deltaTime)
				{
					weather.thunderFlash_ = 1.0f;
					weather.thunderSeed_ = RandomRange(0.0f, 1000.0f);
				}

				const Float thunderDecayRate = 4.0f;
				weather.thunderFlash_ = Max(weather.thunderFlash_ - deltaTime * thunderDecayRate, 0.0f);
			});

		if (!found)
		{
			return;
		}

		Bool cloudFound = false;
		Query<Read<Active>, Write<VolumetricCloudScapes>> cloudQuery(world);
		cloudQuery.ForEach([&](const Active& active, VolumetricCloudScapes& cloud)
			{
				if (!active.active_ || cloudFound)
				{
					return;
				}
				cloudFound = true;

				VolumetricCloudScapes target = cloud;
				GetTarget(weatherType, target);

				cloud.coverage_ = Lerp(cloud.coverage_, target.coverage_, t);
				cloud.lighting_.density_ = Lerp(cloud.lighting_.density_, target.lighting_.density_, t);
				cloud.rain_ = Lerp(cloud.rain_, target.rain_, t);
				cloud.windSpeed_ = Lerp(cloud.windSpeed_, target.windSpeed_, t);
				cloud.sky_.brightness_ = Lerp(cloud.sky_.brightness_, target.sky_.brightness_, t);
			});
	}
}
