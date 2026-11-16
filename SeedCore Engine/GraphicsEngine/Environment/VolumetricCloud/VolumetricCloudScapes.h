#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class VolumetricCloudQuality
	{
		Low,
		Medium,
		High,
		Ultra,
	};

	struct VolumetricCloudShapeSettings
	{
		SC_REFLECTION_CLAMPED_EX("雲の大きさ（m）", 200.0f, 20000.0f)
		Float size_ = 2000.0f;

		SC_REFLECTION_CLAMPED_EX("塊と晴れ間の大きさ（km）", 2.0f, 200.0f)
		Float weatherSize_ = 25.0f;

		SC_REFLECTION_CLAMPED_EX("塊と晴れ間の強さ", 0.0f, 1.0f)
		Float weatherAmount_ = 0.6f;

		SC_REFLECTION_CLAMPED_EX("縁のディテールの細かさ", 1.0f, 16.0f)
		Float detailScale_ = 12.0f;

		SC_REFLECTION_CLAMPED_EX("縁の侵食", 0.0f, 1.0f)
		Float detailStrength_ = 0.6f;

		SC_REFLECTION_CLAMPED_EX("縁の流れる速さ", 0.0f, 2.0f)
		Float detailWindSpeed_ = 0.3f;
	};

	struct VolumetricCloudLightingSettings
	{
		SC_REFLECTION_FIELD_EX("雲の色")
		Color albedo_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		SC_REFLECTION_CLAMPED_EX("濃さ", 0.0f, 1.0f)
		Float density_ = 0.5f;

		SC_REFLECTION_CLAMPED_EX("光の通りやすさ", 0.0f, 1.0f)
		Float lightPenetration_ = 0.6f;

		SC_REFLECTION_CLAMPED_EX("太陽の当たり方", 0.0f, 4.0f)
		Float sunStrength_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("影の明るさ（環境光）", 0.0f, 2.0f)
		Float ambientStrength_ = 0.6f;

		SC_REFLECTION_CLAMPED_EX("縁の光り（前方散乱）", 0.0f, 0.9f)
		Float forwardScattering_ = 0.6f;

		SC_REFLECTION_CLAMPED_EX("もこもこ感（パウダー）", 0.0f, 1.0f)
		Float powderStrength_ = 0.5f;
	};

	struct VolumetricCloudAdvancedSettings
	{
		SC_REFLECTION_CLAMPED_EX("後方散乱の異方性", 0.0f, 0.9f)
		Float phaseBackward_ = 0.35f;

		SC_REFLECTION_CLAMPED_EX("前方・後方のブレンド", 0.0f, 1.0f)
		Float phaseBlend_ = 0.4f;

		SC_REFLECTION_CLAMPED_EX("多重散乱のオクターブ数", 1, 8)
		Uint32 multiScatterOctaves_ = 5;

		SC_REFLECTION_CLAMPED_EX("多重散乱の消散の減衰", 0.1f, 0.95f)
		Float multiScatterAttenuation_ = 0.5f;

		SC_REFLECTION_CLAMPED_EX("多重散乱の寄与の減衰", 0.1f, 0.95f)
		Float multiScatterContribution_ = 0.75f;

		SC_REFLECTION_CLAMPED_EX("多重散乱の偏心の減衰", 0.1f, 0.95f)
		Float multiScatterEccentricity_ = 0.5f;

		SC_REFLECTION_CLAMPED_EX("空気遠近の密度", 0.0f, 0.0002f)
		Float aerialDensity_ = 0.000012f;

		SC_REFLECTION_CLAMPED_EX("IBL の強さ", 0.0f, 4.0f)
		Float iblIntensity_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("惑星の半径", 100000.0f, 40000000.0f)
		Float planetRadius_ = 6360000.0f;
	};

	struct VolumetricCloudSkySettings
	{
		SC_REFLECTION_FIELD_EX("天頂の色")
		Color zenithColor_ = { 0.15f, 0.35f, 0.75f, 1.0f };

		SC_REFLECTION_FIELD_EX("地平線の色")
		Color horizonColor_ = { 0.65f, 0.75f, 0.9f, 1.0f };

		SC_REFLECTION_CLAMPED_EX("空の明るさ", 0.0f, 4.0f)
		Float brightness_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("太陽の視半径", 0.005f, 0.2f)
		Float sunSize_ = 0.03f;

		SC_REFLECTION_FIELD_EX("地面の色")
		Color groundColor_ = { 0.25f, 0.22f, 0.2f, 1.0f };
	};

	struct VolumetricCloudScapes
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲量", 0.0f, 1.0f)
		Float coverage_ = 0.45f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲の形（層雲～積雲）", 0.0f, 1.0f)
		Float cloudType_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雨雲の度合い", 0.0f, 1.0f)
		Float rain_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲の高さ（下端 m）", 0.0f, 5000.0f)
		Float cloudBottom_ = 1500.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲の厚さ（m）", 100.0f, 8000.0f)
		Float cloudThickness_ = 2500.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("風の向き（度）", 0.0f, 360.0f)
		Float windDirection_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("風の速さ（m/s）", 0.0f, 100.0f)
		Float windSpeed_ = 10.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("形")
		VolumetricCloudShapeSettings shape_;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("ライティング")
		VolumetricCloudLightingSettings lighting_;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("品質")
		VolumetricCloudQuality quality_ = VolumetricCloudQuality::High;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("詳細")
		VolumetricCloudAdvancedSettings advanced_;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("空（天体が太陽を動かしていないときだけ効く）")
		VolumetricCloudSkySettings sky_;
	};
	REGISTER_COMPONENT(VolumetricCloudScapes, "Graphics/Environment");
}
