#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct AutoExposureConstantBuffer
	{
		Float minLogLuminance_ = 0.0f;

		Float maxLogLuminance_ = 0.0f;

		Float keyValue_ = 0.0f;

		Float adaptSpeedToBright_ = 0.0f;

		Float adaptSpeedToDark_ = 0.0f;

		StaticArray<Float, 3> padding_ = { 0.0f, 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(AutoExposureConstantBuffer, 32, "PostProcess/AutoExposure/AutoExposure.hlsli");

	struct AutoExposureHistogramDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AutoExposure/AutoExposureHistogramCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 16;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 16;
	};

	struct AutoExposureAverageDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AutoExposure/AutoExposureAverageCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 1;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}
