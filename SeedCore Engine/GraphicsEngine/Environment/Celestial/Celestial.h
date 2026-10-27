#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct SunSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("地平線の色")
		Color horizonColor_ = { 1.0f, 0.55f, 0.25f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("天頂の色")
		Color zenithColor_ = { 1.0f, 0.98f, 0.92f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("最大の強さ", 0.0f, 20.0f)
		Float maxIntensity_ = 3.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("光が消える仰角（度）", -20.0f, 10.0f)
		Float minElevationDegrees_ = -6.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("角半径", 0.005f, 0.2f)
		Float angularRadius_ = 0.03f;
	};

	struct MoonSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("色")
		Color color_ = { 0.65f, 0.72f, 0.85f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("最大の強さ", 0.0f, 2.0f)
		Float maxIntensity_ = 0.15f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("角半径", 0.005f, 0.2f)
		Float angularRadius_ = 0.02f;
	};

	struct Celestial
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("時刻", 0.0f, 24.0f)
		Float hourOfDay_ = 8.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("日", 1, 365)
		Uint32 dayOfMonth_ = 1;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("1 か月の日数", 1, 365)
		Uint32 daysPerMonth_ = 30;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("月", 1, 12)
		Uint32 monthOfYear_ = 1;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("1 日の長さ（分）", 0.1f, 1440.0f)
		Float dayLengthMinutes_ = 20.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("時間の倍率", 0.0f, 100.0f)
		Float timeScale_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("一時停止")
		Bool paused_ = false;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("太陽")
		SunSettings sun_;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("月")
		MoonSettings moon_;
	};
	REGISTER_COMPONENT(Celestial, "Graphics/Environment");
}
