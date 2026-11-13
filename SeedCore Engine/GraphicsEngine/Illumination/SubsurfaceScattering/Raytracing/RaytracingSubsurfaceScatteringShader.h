#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct RaytracingSubsurfaceScatteringConstantBuffer
	{
		Float scatteringDistance_ = 0.5f;

		Float thicknessBias_ = 0.02f;

		Float rayTMax_ = 5.0f;

		Float strength_ = 1.0f;

		Vector3 subsurfaceColor_ = { 1.0f, 0.35f, 0.25f };

		Float raytracingSubsurfaceScatteringConstantBufferPadding0_ = 0.0f;
	};
	SC_STATIC_ASSERT(RaytracingSubsurfaceScatteringConstantBuffer, 32, "Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringTraceCS.hlsl");

	struct RaytracingSubsurfaceScatteringTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringTraceCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
