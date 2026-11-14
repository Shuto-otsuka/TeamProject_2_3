#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct RaytracingAmbientOcclusionConstantBuffer
	{
		Float rayLength_ = 1.0f;

		Float normalBias_ = 0.01f;

		Float power_ = 1.0f;

		Uint32 rayCount_ = 1;

		Uint32 frame_ = 0;

		Uint32 historyValid_ = 0;

		Uint32 raytracingAmbientOcclusionConstantBufferPadding0_ = 0;

		Uint32 raytracingAmbientOcclusionConstantBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(RaytracingAmbientOcclusionConstantBuffer, 32, "Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusion.hlsli");

	struct RaytracingAmbientOcclusionTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionTraceCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingAmbientOcclusionTemporalDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Temporal";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
