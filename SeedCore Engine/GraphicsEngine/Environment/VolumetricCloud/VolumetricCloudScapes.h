#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct VolumetricCloudScapes
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲層の下端", 0.0f, 5000.0f)
		Float cloudBottom_ = 500.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲層の上端", 100.0f, 10000.0f)
		Float cloudTop_ = 1500.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("惑星の半径", 100000.0f, 40000000.0f)
		Float planetRadius_ = 6360000.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲量", 0.0f, 1.0f)
		Float coverage_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("密度", 0.0001f, 0.5f)
		Float densityScale_ = 0.02f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雲の形（層雲〜積雲）", 0.0f, 1.0f)
		Float cloudType_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("雨雲の度合い", 0.0f, 1.0f)
		Float rain_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("形状ノイズのスケール", 0.0001f, 0.01f)
		Float noiseScale_ = 0.0008f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("ディテールのスケール", 1.0f, 16.0f)
		Float detailScale_ = 8.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("ディテールの強さ", 0.0f, 1.0f)
		Float detailStrength_ = 0.35f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("天候フィールドの周波数", 0.005f, 0.5f)
		Float weatherScale_ = 0.06f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("天候フィールドの影響", 0.0f, 1.0f)
		Float weatherAmount_ = 0.6f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("風の速さ", 0.0f, 0.5f)
		Float windSpeed_ = 0.01f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("ディテールの風の速さ", 0.0f, 2.0f)
		Float detailWindSpeed_ = 0.3f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("雲の散乱色")
		Color cloudAlbedo_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("前方散乱の異方性", -0.9f, 0.9f)
		Float scatteringG_ = 0.6f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("後方散乱の異方性", 0.0f, 0.9f)
		Float phaseBackward_ = 0.35f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("前方・後方のブレンド", 0.0f, 1.0f)
		Float phaseBlend_ = 0.4f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("パウダー効果の強さ", 0.0f, 1.0f)
		Float powderStrength_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("環境光の強さ", 0.0f, 2.0f)
		Float ambientStrength_ = 0.4f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("IBL の強さ", 0.0f, 4.0f)
		Float iblIntensity_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("多重散乱のオクターブ数", 1, 8)
		Uint32 multiScatterOctaves_ = 5;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("多重散乱の消散の減衰", 0.1f, 0.95f)
		Float multiScatterAttenuation_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("多重散乱の寄与の減衰", 0.1f, 0.95f)
		Float multiScatterContribution_ = 0.75f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("多重散乱の偏心の減衰", 0.1f, 0.95f)
		Float multiScatterEccentricity_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("視線のステップ数", 8, 256)
		Uint32 stepCount_ = 64;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("ライトマーチのステップ数", 1, 16)
		Uint32 lightStepCount_ = 6;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("マーチの最大距離", 20000.0f, 400000.0f)
		Float maxMarchDistance_ = 120000.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("LOD の開始距離", 2000.0f, 100000.0f)
		Float lodDistance_ = 25000.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("時間方向のジッター", 0.0f, 1.0f)
		Float temporalJitter_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("天頂の色")
		Color skyZenithColor_ = { 0.15f, 0.35f, 0.75f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("地平線の色")
		Color skyHorizonColor_ = { 0.65f, 0.75f, 0.9f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("空の明るさ", 0.0f, 4.0f)
		Float skyBrightness_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("太陽の視半径", 0.005f, 0.2f)
		Float sunSize_ = 0.03f;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("地面の色")
		Color groundColor_ = { 0.25f, 0.22f, 0.2f, 1.0f };

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("空気遠近の密度", 0.0f, 0.0002f)
		Float aerialDensity_ = 0.000012f;
	};
	REGISTER_COMPONENT(VolumetricCloudScapes, "Graphics/Environment");
}
