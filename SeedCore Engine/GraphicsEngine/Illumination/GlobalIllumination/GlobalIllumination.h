#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class GlobalIlluminationTechnique
	{
		Raytracing,
		DynamicDiffuse,
		LightPropagationVolumes,
		ScreenSpace,
	};

	struct RaytracingGlobalIlluminationSettings
	{
		SC_REFLECTION_CLAMPED_EX("レイの本数", 1, 16)
		Uint32 rayCount_ = 1;

		SC_REFLECTION_CLAMPED_EX("バウンス数", 1, 4)
		Uint32 bounceCount_ = 1;

		SC_REFLECTION_CLAMPED_EX("レイの最大長", 50.0f, 20000.0f)
		Float rayTMax_ = 2000.0f;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0f, 1.0f)
		Float normalBias_ = 0.05f;
	};

	struct DynamicDiffuseGlobalIlluminationSettings
	{
		SC_REFLECTION_CLAMPED_EX("プローブあたりのレイ数", 32, 512)
		Uint32 raysPerProbe_ = 128;

		SC_REFLECTION_CLAMPED_EX("プローブの間隔", 0.25f, 20.0f)
		Float probeSpacing_ = 2.0f;

		SC_REFLECTION_CLAMPED_EX("履歴の重み", 0.5f, 0.999f)
		Float hysteresis_ = 0.97f;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0f, 1.0f)
		Float normalBias_ = 0.25f;
	};

	struct LightPropagationVolumesSettings
	{
		SC_REFLECTION_CLAMPED_EX("格子の解像度", 16, 64)
		Uint32 gridResolution_ = 32;

		SC_REFLECTION_CLAMPED_EX("ボリュームの大きさ", 8.0f, 512.0f)
		Float volumeSize_ = 64.0f;

		SC_REFLECTION_CLAMPED_EX("カスケードの数", 1, 4)
		Uint32 cascadeCount_ = 3;

		SC_REFLECTION_CLAMPED_EX("伝播の回数", 1, 32)
		Uint32 propagationIterations_ = 8;

		SC_REFLECTION_CLAMPED_EX("RSM（反射シャドウマップ）の解像度", 128, 2048)
		Uint32 reflectiveShadowMapResolution_ = 512;

		SC_REFLECTION_CLAMPED_EX("注入のオフセット", 0.0f, 2.0f)
		Float injectionBias_ = 0.5f;

		SC_REFLECTION_FIELD_EX("遮蔽")
		Bool occlusionEnabled_ = true;

		SC_REFLECTION_FIELD_CONDITION(occlusionEnabled_)
		SC_REFLECTION_CLAMPED_EX("遮蔽の強さ", 0.0f, 2.0f)
		Float occlusionStrength_ = 1.0f;
	};

	struct ScreenSpaceGlobalIlluminationSettings
	{
		SC_REFLECTION_CLAMPED_EX("レイの長さ", 0.1f, 20.0f)
		Float rayLength_ = 2.0f;

		SC_REFLECTION_CLAMPED_EX("厚み", 0.01f, 2.0f)
		Float thickness_ = 0.25f;

		SC_REFLECTION_CLAMPED_EX("サンプル数", 2, 64)
		Uint32 sampleCount_ = 8;
	};

	struct GlobalIllumination
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		GlobalIlluminationTechnique technique_ = GlobalIlluminationTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("間接光の強さ", 0.0f, 5.0f)
		Float intensity_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == GlobalIlluminationTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシングGI")
		RaytracingGlobalIlluminationSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == GlobalIlluminationTechnique::DynamicDiffuse)
		SC_REFLECTION_FIELD_EX("動的ディフューズGI（DDGI）")
		DynamicDiffuseGlobalIlluminationSettings dynamicDiffuse_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == GlobalIlluminationTechnique::LightPropagationVolumes)
		SC_REFLECTION_FIELD_EX("ライト・プロパゲーション・ボリューム（LPV）")
		LightPropagationVolumesSettings lightPropagationVolumes_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == GlobalIlluminationTechnique::ScreenSpace)
		SC_REFLECTION_FIELD_EX("スクリーンスペースGI（SSGI）")
		ScreenSpaceGlobalIlluminationSettings screenSpace_;
	};
	REGISTER_COMPONENT(GlobalIllumination, "Graphics/Illumination");
}