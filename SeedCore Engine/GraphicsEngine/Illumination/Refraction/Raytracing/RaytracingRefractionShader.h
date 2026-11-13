#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct RaytracingRefractionConstantBuffer
	{
		Float rayTMax_ = 1000.0f;

		Float normalBias_ = 0.01f;

		Float strength_ = 1.0f;

		Float raytracingRefractionConstantBufferPadding0_ = 0.0f;
	};
	SC_STATIC_ASSERT(RaytracingRefractionConstantBuffer, 16, "Illumination/Refraction/Raytracing/RaytracingRefractionTraceCS.hlsl");

	struct RaytracingRefractionTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionTraceCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
