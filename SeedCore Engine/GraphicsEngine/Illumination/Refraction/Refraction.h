#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class RefractionTechnique
	{
		Raytracing,
		ScreenSpace,
	};

	struct RaytracingRefractionSettings
	{
		SC_REFLECTION_CLAMPED_EX("レイの最大長", 1.0f, 5000.0f)
		Float rayTMax_ = 1000.0f;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0001f, 1.0f)
		Float normalBias_ = 0.01f;
	};

	struct ScreenSpaceRefractionSettings
	{
		
	};

	struct Refraction
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		RefractionTechnique technique_ = RefractionTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("屈折の強さ", 0.0f, 1.0f)
		Float strength_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == RefractionTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシング屈折")
		RaytracingRefractionSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == RefractionTechnique::ScreenSpace)
		SC_REFLECTION_FIELD_EX("スクリーンスペース屈折")
		ScreenSpaceRefractionSettings screenSpace_;
	};
	REGISTER_COMPONENT(Refraction, "Graphics/Illumination");
}