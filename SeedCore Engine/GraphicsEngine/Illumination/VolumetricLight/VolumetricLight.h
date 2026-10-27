#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class VolumetricLightTechnique
	{
		Raytracing,
		HeightFog,
	};

	struct RaytracingVolumetricLightSettings
	{
		SC_REFLECTION_CLAMPED_EX("散乱の異方性", 0.0f, 0.95f)
		Float scatteringG_ = 0.7f;

		SC_REFLECTION_CLAMPED_EX("シャドウレイの最大長", 10.0f, 10000.0f)
		Float rayTMax_ = 2000.0f;

		SC_REFLECTION_CLAMPED_EX("ゴッドレイの強さ", 0.0f, 8.0f)
		Float godrayStrength_ = 1.0f;

		SC_REFLECTION_FIELD_EX("雲の影")
		Bool cloudShadowEnabled_ = true;
	};

	struct HeightFogSettings
	{

	};

	struct VolumetricLight
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		VolumetricLightTechnique technique_ = VolumetricLightTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("フォグの密度", 0.0001f, 0.5f)
		Float density_ = 0.003f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("吸収", 0.0f, 0.2f)
		Float absorption_ = 0.0005f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("高さによる減衰", 0.0f, 0.5f)
		Float heightFalloff_ = 0.05f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("基準の高さ", -100.0f, 500.0f)
		Float heightReference_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("フォグの散乱色")
		Color fogAlbedo_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == VolumetricLightTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシングボリュームライト")
		RaytracingVolumetricLightSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == VolumetricLightTechnique::HeightFog)
		SC_REFLECTION_FIELD_EX("高さフォグ")
		HeightFogSettings heightFog_;
	};
	REGISTER_COMPONENT(VolumetricLight, "Graphics/Illumination");
}
