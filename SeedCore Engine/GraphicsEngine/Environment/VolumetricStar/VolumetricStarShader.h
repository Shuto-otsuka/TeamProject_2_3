#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct VolumetricStarMeteor
	{
		Vector3 startDirection_ = { 0.0f, 1.0f, 0.0f };

		Float progress_ = 0.0f;

		Vector3 endDirection_ = { 0.0f, 1.0f, 0.0f };

		Float brightness_ = 0.0f;
	};
	SC_STATIC_ASSERT(VolumetricStarMeteor, 32, "Environment/VolumetricStar/VolumetricStar.hlsli");

	struct VolumetricStarConstantBuffer
	{
		Float cellSize_ = 0.12f;

		Float density_ = 0.35f;

		Float brightness_ = 1.0f;

		Float twinkleSpeed_ = 2.0f;

		Vector3 color_ = { 0.9f, 0.95f, 1.0f };

		Float sizeMin_ = 0.02f;

		Float sizeMax_ = 0.05f;

		Float shootingStarChancePerSecond_ = 0.05f;

		Float shootingStarBrightness_ = 3.0f;

		Float shootingStarWidth_ = 0.0015f;

		Uint maxConcurrentShootingStars_ = 4;

		Uint enabled_ = 0;

		Float glowIntensity_ = 0.8f;

		Float glowFalloff_ = 120.0f;

		StaticArray<VolumetricStarMeteor, 4> activeShootingStars_ = {};
	};
	SC_STATIC_ASSERT(VolumetricStarConstantBuffer, 192, "Environment/VolumetricStar/VolumetricStar.hlsli");

	struct VolumetricStarTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Environment/VolumetricStar/VolumetricStarCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Trace";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
