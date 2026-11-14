#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/RaytracingPipeline.h>

namespace SeedCore
{
	struct RaytracingGlobalIlluminationConstantBuffer
	{
		Float rayTMax_ = 2000.0f;

		Float normalBias_ = 0.05f;

		Float intensity_ = 1.0f;

		Uint32 rayCount_ = 1;

		Uint32 bounceCount_ = 1;

		Uint32 frame_ = 0;

		Uint32 temporalReuse_ = 1;

		Uint32 historyValid_ = 0;
	};
	SC_STATIC_ASSERT(RaytracingGlobalIlluminationConstantBuffer, 32, "Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIllumination.hlsli");

	struct RaytracingGlobalIlluminationReservoir
	{
		Vector3 samplePosition_ = { 0.0f, 0.0f, 0.0f };

		Float sampleM_ = 0.0f;

		Vector3 sampleNormal_ = { 0.0f, 0.0f, 0.0f };

		Float sampleW_ = 0.0f;

		Vector3 sampleRadiance_ = { 0.0f, 0.0f, 0.0f };

		Float sampleAge_ = 0.0f;

		Vector3 receiverNormal_ = { 0.0f, 0.0f, 0.0f };

		Float receiverDepth_ = 0.0f;
	};
	SC_STATIC_ASSERT(RaytracingGlobalIlluminationReservoir, 64, "Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationReSTIR.hlsli");

	struct RaytracingGlobalIlluminationTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationTraceRT.hlsl");

		SC_CONSTEX const Char* rayGeneration_ = "RaytracingGlobalIlluminationRayGeneration";

		SC_CONSTEX const Char* miss_ = "RaytracingGlobalIlluminationMiss";

		SC_CONSTEX const Char* closestHit_ = "RaytracingGlobalIlluminationClosestHit";

		SC_CONSTEX const Char* anyHit_ = "RaytracingGlobalIlluminationAnyHit";

		SC_CONSTEX const Char* hitGroup_ = "RaytracingGlobalIlluminationHitGroup";

		SC_CONSTEX Uint32 payloadSize_ = 36;

		SC_CONSTEX Uint32 recursionDepth_ = 1;
	};

	struct RaytracingGlobalIlluminationSpatialDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationSpatialCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingGlobalIlluminationTemporalDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Temporal";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingGlobalIlluminationATrousDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "ATrous";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
