#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class SubsurfaceScatteringTechnique
	{
		Raytracing,
		ScreenSpace,
	};

	struct RaytracingSubsurfaceScatteringSettings
	{
		SC_REFLECTION_CLAMPED_EX("最大の厚み", 0.1f, 50.0f)
		Float rayTMax_ = 5.0f;

		SC_REFLECTION_CLAMPED_EX("厚み計算のオフセット", 0.0001f, 1.0f)
		Float thicknessBias_ = 0.02f;
	};

	struct ScreenSpaceSubsurfaceScatteringSettings
	{

	};

	struct SubsurfaceScattering
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		SubsurfaceScatteringTechnique technique_ = SubsurfaceScatteringTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("散乱距離", 0.01f, 5.0f)
		Float scatteringDistance_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("透光の強さ", 0.0f, 4.0f)
		Float strength_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("透けた光の色")
		Color subsurfaceColor_ = { 1.0f,0.35f,0.25f,1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == SubsurfaceScatteringTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシングサブサーフェススキャターリング")
		RaytracingSubsurfaceScatteringSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == SubsurfaceScatteringTechnique::ScreenSpace)
		SC_REFLECTION_FIELD_EX("スクリーンスペースサブサーフェススキャターリング")
		ScreenSpaceSubsurfaceScatteringSettings screenSpace_;
	};
	REGISTER_COMPONENT(SubsurfaceScattering, "Graphics/Illumination");
}