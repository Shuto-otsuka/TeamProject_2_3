#pragma once
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	/**
	* [EN]
	* Kind of a LightList entry. Mirrors LIGHT_TYPE_* in Light/Light.hlsli.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* LightList の 1 件の種類。Light/Light.hlsli の LIGHT_TYPE_* と対応する。
	*/
	enum class LightType :Uint32
	{
		Point,
		Spot,
		Rect,
	};

	/**
	* [EN]
	* One entry of LightList, shared by point, spot and rectangle lights and
	* told apart by type_ - see Light.hlsli's LightStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* LightList の 1 件。点光源・スポットライト・矩形ライトで共通にし、type_ で
	* 区別する。Light.hlsli の LightStructuredBuffer を参照。
	*/
	struct LightStructuredBuffer
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Float range_ = 0.0f;

		Vector3 color_ = { 0.0f, 0.0f, 0.0f };
		Float intensity_ = 0.0f;

		Vector3 direction_ = { 0.0f, 0.0f, 0.0f };
		LightType type_ = LightType::Point;

		Vector3 right_ = { 0.0f, 0.0f, 0.0f };
		Float halfWidth_ = 0.0f;

		Vector3 up_ = { 0.0f, 0.0f, 0.0f };
		Float halfHeight_ = 0.0f;

		Float cosHalfAngle_ = 0.0f;
		Float softness_ = 0.0f;
		Uint shadowIndex_ = SC_INVALID;
		Uint lightStructuredBufferPadding0_ = 0;
	};
	SC_STATIC_ASSERT(LightStructuredBuffer, 96, "Light/Light.hlsli");

	/**
	* [EN]
	* LightConstants: the sun and moon, the light count, the night factor and
	* the sky light's intensity - see Light.hlsli's LightConstantBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* LightConstants。太陽と月、ライトの数、夜の度合い、空の光の強さ。
	* Light.hlsli の LightConstantBuffer を参照。
	*/
	struct LightConstantBuffer
	{
		Vector3 sunDirection_ = { 0.0f, -1.0f, 0.0f };
		Float sunIntensity_ = 0.0f;

		Vector3 sunColor_ = { 0.0f, 0.0f, 0.0f };
		Float sunAngularRadius_ = 0.02f;

		Vector3 moonColor_ = { 0.0f, 0.0f, 0.0f };
		Float moonIntensity_ = 0.0f;

		Float moonAngularRadius_ = 0.02f;
		Float moonPhase_ = 0.0f;
		Float nightFactor_ = 0.0f;
		Uint lightCount_ = 0;

		Float skyIntensity_ = 0.0f;
		Uint lightConstantPadding0_[3] = { 0, 0, 0 };
	};
	SC_STATIC_ASSERT(LightConstantBuffer, 80, "Light/Light.hlsli");
}
