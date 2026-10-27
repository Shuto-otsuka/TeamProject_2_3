#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct VolumetricStar
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("セルのサイズ", 0.02f, 0.5f)
		Float cellSize_ = 0.12f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("星の密度", 0.0f, 1.0f)
		Float density_ = 0.35f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("明るさ", 0.0f, 5.0f)
		Float brightness_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("IBL の強さ", 0.0f, 4.0f)
		Float iblIntensity_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("またたく速さ", 0.0f, 10.0f)
		Float twinkleSpeed_ = 2.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("星の色")
		Color color_ = { 0.9f, 0.95f, 1.0f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("最小サイズ", 0.005f, 0.2f)
		Float sizeMin_ = 0.02f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("最大サイズ", 0.005f, 0.2f)
		Float sizeMax_ = 0.05f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("グローの強さ", 0.0f, 3.0f)
		Float glowIntensity_ = 0.8f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("グローの減衰", 10.0f, 400.0f)
		Float glowFalloff_ = 120.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("流れ星の出現確率（毎秒）", 0.0f, 1.0f)
		Float shootingStarChancePerSecond_ = 0.05f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("流れ星の明るさ", 0.0f, 10.0f)
		Float shootingStarBrightness_ = 3.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("流れ星の幅", 0.0002f, 0.02f)
		Float shootingStarWidth_ = 0.0015f;
	};
	REGISTER_COMPONENT(VolumetricStar, "Graphics/Environment");
}
