#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class AmbientOcclusionTechnique
	{
		Raytracing,
		GroundTruth,
		ScreenSpace,
	};

	struct RaytracingAmbientOcclusionSettings
	{
		SC_REFLECTION_CLAMPED_EX("レイの本数", 1, 32)
		Uint32 rayCount_ = 1;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0001f, 1.0f)
		Float normalBias_ = 0.01f;
	};

	struct GroundTruthAmbientOcclusionSettings
	{
		SC_REFLECTION_CLAMPED_EX("減衰の範囲", 0.0f, 1.0f)
		Float falloffRange_ = 0.165f;

		SC_REFLECTION_CLAMPED_EX("スライス数", 1, 8)
		Uint32 sliceCount_ = 3;

		SC_REFLECTION_CLAMPED_EX("スライスあたりのステップ数", 1, 8)
		Uint32 stepsPerSlice_ = 3;
	};

	struct ScreenSpaceAmbientOcclusionSettings
	{
		SC_REFLECTION_CLAMPED_EX("バイアス", 0.0f, 0.2f)
		Float bias_ = 0.025f;

		SC_REFLECTION_CLAMPED_EX("強さ", 0.0f, 4.0f)
		Float intensity_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("サンプル数", 4, 128)
		Uint32 sampleCount_ = 16;
	};

	struct AmbientOcclusion
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		AmbientOcclusionTechnique technique_ = AmbientOcclusionTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("遮蔽の半径", 0.05f, 10.0f)
		Float radius_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("コントラスト", 0.25f, 5.0f)
		Float power_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == AmbientOcclusionTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシングAO")
		RaytracingAmbientOcclusionSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == AmbientOcclusionTechnique::GroundTruth)
		SC_REFLECTION_FIELD_EX("グラウンドトゥルースAO（SSAO）")
		GroundTruthAmbientOcclusionSettings groundTruth_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == AmbientOcclusionTechnique::ScreenSpace)
		SC_REFLECTION_FIELD_EX("スクリーンスペースAO（SSAO）")
		ScreenSpaceAmbientOcclusionSettings screenSpace_;
	};
	REGISTER_COMPONENT(AmbientOcclusion, "Graphics/Illumination");
}