#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class ShadowTechnique
	{
		Raytracing,
		VirtualShadowMap,
		CascadedShadowMap,
	};

	struct RaytracingShadowSettings
	{
		SC_REFLECTION_CLAMPED_EX("レイの本数", 1, 32)
		Uint32 rayCount_ = 1;

		SC_REFLECTION_CLAMPED_EX("レイの最大長", 1.0f, 5000.0f)
		Float rayTMax_ = 1000.0f;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0001f, 1.0f)
		Float normalBias_ = 0.01f;

		SC_REFLECTION_CLAMPED_EX("影の濃さ", 0.0f, 2.0f)
		Float shadowStrength_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("太陽の角半径", 0.0f, 0.2f)
		Float sunAngularRadius_ = 0.02f;

		SC_REFLECTION_CLAMPED_EX("点光源の半径", 0.0f, 2.0f)
		Float punctualLightRadius_ = 0.1f;
	};

	struct VirtualShadowMapSettings
	{
		SC_REFLECTION_CLAMPED_EX("仮想解像度（2の累乗）", 11, 16)
		Uint32 virtualResolutionLog2_ = 14;

		SC_REFLECTION_CLAMPED_EX("ページプールのサイズ（MiB）", 16, 512)
		Uint32 pagePoolSizeMib_ = 128;

		SC_REFLECTION_CLAMPED_EX("クリップマップの段数", 1, 12)
		Uint32 clipmapLevelCount_ = 6;

		SC_REFLECTION_CLAMPED_EX("柔らかさの半径", 0.0f, 4.0f)
		Float softnessRadius_ = 0.5f;

		SC_REFLECTION_CLAMPED_EX("深度バイアス", 0.0001f, 0.05f)
		Float depthBias_ = 0.002f;
	};

	struct CascadedShadowMapSettings
	{
		SC_REFLECTION_CLAMPED_EX("カスケードの数", 1, 4)
		Uint32 cascadeCount_ = 4;

		SC_REFLECTION_CLAMPED_EX("分割の比率", 0.0f, 1.0f)
		Float splitLambda_ = 0.85f;

		SC_REFLECTION_CLAMPED_EX("PCFのカーネルサイズ", 1, 9)
		Uint32 pcfKernelSize_ = 3;

		SC_REFLECTION_CLAMPED_EX("カスケード境界のブレンド幅", 0.0f, 0.5f)
		Float cascadeBlendWidth_ = 0.1f;

		SC_REFLECTION_CLAMPED_EX("深度バイアス", 0.0001f, 0.05f)
		Float depthBias_ = 0.002f;

		SC_REFLECTION_FIELD_EX("ちらつき防止")
		Bool stabilize_ = true;
	};

	struct Shadow
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		ShadowTechnique technique_ = ShadowTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_ && technique_ == ShadowTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシング影")
		RaytracingShadowSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_ && technique_ == ShadowTechnique::VirtualShadowMap)
		SC_REFLECTION_FIELD_EX("バーチャルシャドウマップ（VSM）")
		VirtualShadowMapSettings virtualShadowMap_;

		SC_REFLECTION_FIELD_CONDITION(enable_ && technique_ == ShadowTechnique::CascadedShadowMap)
		SC_REFLECTION_FIELD_EX("カスケードシャドウマップ（CSM）")
		CascadedShadowMapSettings cascadedShadowMap_;
	};
	REGISTER_COMPONENT(Shadow, "Graphics/Illumination");
}