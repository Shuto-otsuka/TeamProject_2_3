#pragma once
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	enum class WeatherType
	{
		Clear,
		Cloudy,
		Overcast,
		Rain,
		Snow,
		Storm,
	};

	struct Rain
	{
		SC_REFLECTION_CLAMPED_EX("密度", 0.0f, 1.0f)
		Float density_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("落下速度", 1.0f, 30.0f)
		Float fallSpeed_ = 9.0f;

		SC_REFLECTION_CLAMPED_EX("サイズ", 0.005f, 0.2f)
		Float size_ = 0.035f;

		SC_REFLECTION_CLAMPED_EX("筋の長さ", 0.0f, 2.0f)
		Float streakLength_ = 0.035f;

		SC_REFLECTION_CLAMPED_EX("明るさ", 0.0f, 3.0f)
		Float brightness_ = 0.8f;

		SC_REFLECTION_CLAMPED_EX("スポーン範囲(半径)", 2.0f, 60.0f)
		Float volumeRadius_ = 14.0f;

		SC_REFLECTION_CLAMPED_EX("スポーン範囲(高さ)", 2.0f, 40.0f)
		Float volumeHeight_ = 10.0f;

		SC_REFLECTION_FIELD_EX("色")
		Color color_ = { 0.75f, 0.8f, 0.9f, 1.0f };
	};

	struct Snow
	{
		SC_REFLECTION_CLAMPED_EX("密度", 0.0f, 1.0f)
		Float density_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("落下速度", 0.1f, 10.0f)
		Float fallSpeed_ = 1.3f;

		SC_REFLECTION_CLAMPED_EX("サイズ", 0.005f, 0.3f)
		Float size_ = 0.05f;

		SC_REFLECTION_CLAMPED_EX("揺れ幅", 0.0f, 2.0f)
		Float swayAmount_ = 0.4f;

		SC_REFLECTION_CLAMPED_EX("明るさ", 0.0f, 3.0f)
		Float brightness_ = 0.9f;

		SC_REFLECTION_CLAMPED_EX("スポーン範囲(半径)", 2.0f, 60.0f)
		Float volumeRadius_ = 12.0f;

		SC_REFLECTION_CLAMPED_EX("スポーン範囲(高さ)", 2.0f, 40.0f)
		Float volumeHeight_ = 9.0f;

		SC_REFLECTION_FIELD_EX("色")
		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct Weather
	{
		SC_REFLECTION_FIELD_EX("天候")
		WeatherType type_ = WeatherType::Clear;

		SC_REFLECTION_FIELD_EX("自動サイクル")
		Bool autoCycle_ = false;

		SC_REFLECTION_CLAMPED_EX("最短間隔(分)", 0.1f, 180.0f)
		Float minIntervalMinutes_ = 5.0f;

		SC_REFLECTION_CLAMPED_EX("最長間隔(分)", 0.1f, 180.0f)
		Float maxIntervalMinutes_ = 15.0f;

		SC_REFLECTION_CLAMPED_EX("遷移時間(秒)", 0.1f, 300.0f)
		Float transitionSeconds_ = 20.0f;

		SC_REFLECTION_FIELD_EX("雨を季節無視で許可")
		Bool forceRain_ = false;

		SC_REFLECTION_FIELD_EX("雪を季節無視で許可")
		Bool forceSnow_ = false;

		SC_REFLECTION_FIELD_EX("雷を強制的に許可")
		Bool forceThunder_ = false;

		SC_REFLECTION_FIELD_EX("暴風を強制的に許可")
		Bool forceStorm_ = false;

		SC_REFLECTION_FIELD_EX("雨パーティクル")
		Bool rainEnabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(rainEnabled_)
		SC_REFLECTION_FIELD_EX("雨")
		Rain rain_;

		SC_REFLECTION_FIELD_EX("雪パーティクル")
		Bool snowEnabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(snowEnabled_)
		SC_REFLECTION_FIELD_EX("雪")
		Snow snow_;

		Float autoCycleTimer_ = 0.0f;

		Float wetness_ = 0.0f;

		Float snowCoverage_ = 0.0f;

		Float thunderFlash_ = 0.0f;

		Float thunderSeed_ = 0.0f;

		Float snowIntensity_ = 0.0f;
	};
	REGISTER_COMPONENT(Weather, "Environment", ComponentStorage::SparseSet);
}
