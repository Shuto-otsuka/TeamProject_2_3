#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct DepthOfFieldSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("焦点距離", 0.0f, 100.0f)
		Float focusDistance_ = 10.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("焦点範囲", 0.01f, 50.0f)
		Float focusRange_ = 4.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("最大ぼけ半径", 0.0f, 0.05f)
		Float maxBlurRadius_ = 0.015f;
	};

	struct BokehSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("しきい値", 0.0f, 20.0f)
		Float highlightThreshold_ = 4.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強度", 0.0f, 5.0f)
		Float highlightIntensity_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("角形の頂点数", 3, 8)
		Uint32 bladeCount_ = 6;
	};

	struct LensDistortionSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("歪曲 k1", -0.5f, 0.5f)
		Float k1_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("歪曲 k2", -0.5f, 0.5f)
		Float k2_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("歪曲 k3", -0.5f, 0.5f)
		Float k3_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("スケール", 0.5f, 1.5f)
		Float scale_ = 1.0f;
	};

	struct ChromaticAberrationSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強度", 0.0f, 0.05f)
		Float intensity_ = 0.008f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("サンプル数", 3, 16)
		Uint32 sampleCount_ = 8;
	};

	struct VignetteSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強度", 0.0f, 8.0f)
		Float intensity_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("減光指数", 1.0f, 8.0f)
		Float exponent_ = 4.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("色")
		Color color_ = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct BloomSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("しきい値", 0.0f, 10.0f)
		Float threshold_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("ソフトニー", 0.0f, 1.0f)
		Float softKnee_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強度", 0.0f, 1.0f)
		Float intensity_ = 0.08f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("フィルタ半径", 0.001f, 0.02f)
		Float filterRadius_ = 0.005f;
	};

	struct AnamorphicFlareSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("しきい値", 0.0f, 20.0f)
		Float threshold_ = 4.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強度", 0.0f, 5.0f)
		Float intensity_ = 0.6f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("筋の長さ", 0.0f, 1.0f)
		Float streakLength_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("減衰", 0.80f, 0.99f)
		Float attenuation_ = 0.95f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("色")
		Color tint_ = { 0.35f, 0.6f, 1.0f, 1.0f };
	};

	struct LensFlareSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("しきい値", 0.0f, 20.0f)
		Float threshold_ = 4.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("全体強度", 0.0f, 5.0f)
		Float intensity_ = 0.5f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("スパイク長", 0.0f, 1.0f)
		Float streakLength_ = 0.25f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("減衰", 0.80f, 0.99f)
		Float streakAttenuation_ = 0.93f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("色収差", 0.0f, 0.02f)
		Float chromaticAberration_ = 0.004f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("回転", -3.14159265f, 3.14159265f)
		Float angleOffset_ = 0.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("ゴースト数", 1, 8)
		Uint32 ghostCount_ = 4;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("ゴースト間隔", 0.0f, 1.0f)
		Float ghostDispersal_ = 0.3f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("ゴースト強度", 0.0f, 2.0f)
		Float ghostIntensity_ = 0.3f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("ハロー半径", 0.0f, 1.0f)
		Float haloWidth_ = 0.45f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("棘のばらつき", 0.0f, 1.0f)
		Float spikeVariation_ = 0.35f;
	};

	struct ExposureSettings
	{
		SC_REFLECTION_CLAMPED_EX("露出補正(EV)", -8.0f, 8.0f)
		Float compensation_ = 1.65f;

		SC_REFLECTION_FIELD_EX("自動露出(ヒストグラム)を使う")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("最小log輝度", -16.0f, 0.0f)
		Float minLogLuminance_ = -10.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("最大log輝度", 0.0f, 16.0f)
		Float maxLogLuminance_ = 4.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("目標輝度(キー値)", 0.001f, 1.0f)
		Float keyValue_ = 0.18f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("明順応速度", 0.01f, 20.0f)
		Float adaptSpeedToBright_ = 3.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("暗順応速度", 0.01f, 20.0f)
		Float adaptSpeedToDark_ = 1.0f;
	};

	struct ColorGradingRangeSettings
	{
		SC_REFLECTION_CLAMPED_EX("色温度", -1.0f, 1.0f)
		Float temperature_ = 0.0f;

		SC_REFLECTION_CLAMPED_EX("彩度", 0.0f, 4.0f)
		Float saturation_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("コントラスト", 0.1f, 4.0f)
		Float contrast_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("ガンマ", 0.1f, 4.0f)
		Float gamma_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("ゲイン", 0.0f, 4.0f)
		Float gain_ = 1.0f;

		SC_REFLECTION_CLAMPED_EX("オフセット", -1.0f, 1.0f)
		Float offset_ = 0.0f;
	};

	struct ColorGradingSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("全体")
		ColorGradingRangeSettings global_;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("シャドウ")
		ColorGradingRangeSettings shadows_;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("中間調")
		ColorGradingRangeSettings midtones_;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("ハイライト")
		ColorGradingRangeSettings highlights_;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("シャドウ上限", 0.0f, 1.0f)
		Float shadowsMax_ = 0.09f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("ハイライト下限", 0.0f, 2.0f)
		Float highlightsMin_ = 0.5f;
	};

	struct ToneMappingSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = true;

		enum class ToneCurve
		{
			None,

			Reinhard,

			AcesFilmic,

			PbrNeutral
		};

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("方式")
		ToneCurve mode_ = ToneCurve::AcesFilmic;
	};

	struct SharpnessSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強さ", 0.0f, 2.0f)
		Float amount_ = 0.5f;
	};

	struct FilmGrainSettings
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = false;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("強度", 0.0f, 1.0f)
		Float intensity_ = 0.15f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("粒の大きさ", 1.0f, 8.0f)
		Float size_ = 2.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_CLAMPED_EX("階調応答", 0.0f, 1.0f)
		Float luminanceResponse_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enabled_)
		SC_REFLECTION_FIELD_EX("色付き")
		Bool colored_ = true;
	};

	struct PostProcess
	{
		SC_REFLECTION_FIELD_EX("被写界深度")
		DepthOfFieldSettings depthOfField_;

		SC_REFLECTION_FIELD_EX("ボケ")
		BokehSettings bokeh_;

		SC_REFLECTION_FIELD_EX("レンズ歪曲")
		LensDistortionSettings lensDistortion_;

		SC_REFLECTION_FIELD_EX("色収差")
		ChromaticAberrationSettings chromaticAberration_;

		SC_REFLECTION_FIELD_EX("ビネット")
		VignetteSettings vignette_;

		SC_REFLECTION_FIELD_EX("ブルーム")
		BloomSettings bloom_;

		SC_REFLECTION_FIELD_EX("アナモルフィックフレア")
		AnamorphicFlareSettings anamorphicFlare_;

		SC_REFLECTION_FIELD_EX("レンズフレア")
		LensFlareSettings lensFlare_;

		SC_REFLECTION_FIELD_EX("露出")
		ExposureSettings exposure_;

		SC_REFLECTION_FIELD_EX("カラーグレーディング")
		ColorGradingSettings colorGrading_;

		SC_REFLECTION_FIELD_EX("トーンマップ")
		ToneMappingSettings toneMapping_;

		SC_REFLECTION_FIELD_EX("シャープネス")
		SharpnessSettings sharpness_;

		SC_REFLECTION_FIELD_EX("フィルムグレイン")
		FilmGrainSettings filmGrain_;
	};
	REGISTER_COMPONENT(PostProcess, "PostProcess");
}
