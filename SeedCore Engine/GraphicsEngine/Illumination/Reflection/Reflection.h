#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class ReflectionTechnique
	{
		Raytracing,
		SignedDistanceField,
		ScreenSpace,
	};

	struct RaytracingReflectionSettings
	{
		SC_REFLECTION_CLAMPED_EX("レイの本数", 1, 32)
		Uint32 rayCount_ = 1;

		SC_REFLECTION_CLAMPED_EX("バウンス数", 1, 4)
		Uint32 bounceCount_ = 1;

		SC_REFLECTION_CLAMPED_EX("バウンス後の最大の粗さ", 0.0f, 1.0f)
		Float bounceRoughnessThreshold_ = 0.3f;

		SC_REFLECTION_CLAMPED_EX("レイの最大長", 1.0f, 5000.0f)
		Float rayTMax_ = 1000.0f;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0001f, 1.0f)
		Float normalBias_ = 0.01f;
	};

	struct SignedDistanceFieldReflectionSettings
	{
		SC_REFLECTION_CLAMPED_EX("最大距離", 1.0f, 5000.0f)
		Float maxDistance_ = 1000.0f;

		SC_REFLECTION_CLAMPED_EX("コーンの角度", 0.0f, 0.5f)
		Float coneAngle_ = 0.05f;

		SC_REFLECTION_CLAMPED_EX("法線方向のオフセット", 0.0001f, 1.0f)
		Float normalBias_ = 0.01f;
	};

	struct ScreenSpaceReflectionSettings
	{
		SC_REFLECTION_CLAMPED_EX("反射する最大の粗さ", 0.0f, 1.0f)
		Float maxRoughness_ = 0.6f;

		SC_REFLECTION_CLAMPED_EX("厚み", 0.01f, 2.0f)
		Float thickness_ = 0.25f;

		SC_REFLECTION_CLAMPED_EX("最大ステップ数", 8, 256)
		Uint32 maxStepCount_ = 64;
	};

	struct Reflection
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		ReflectionTechnique technique_ = ReflectionTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_CLAMPED_EX("反射の強さ", 0.0f, 5.0f)
		Float strength_ = 1.0f;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == ReflectionTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシング反射")
		RaytracingReflectionSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == ReflectionTechnique::SignedDistanceField)
		SC_REFLECTION_FIELD_EX("距離フィールド反射（SDFR）")
		SignedDistanceFieldReflectionSettings signedDistanceField_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == ReflectionTechnique::ScreenSpace)
		SC_REFLECTION_FIELD_EX("スクリーンスペース反射（SSR）")
		ScreenSpaceReflectionSettings screenSpace_;
	};
	REGISTER_COMPONENT(Reflection, "Graphics/Illumination");
}