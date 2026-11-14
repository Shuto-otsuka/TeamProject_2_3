#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/RaytracingPipeline.h>

namespace SeedCore
{
	struct RaytracingReflectionConstantBuffer
	{
		Float rayTMax_ = 1000.0f;

		Float normalBias_ = 0.01f;

		Float strength_ = 1.0f;

		Uint32 rayCount_ = 1;

		Uint32 frame_ = 0;

		Uint32 temporalReuse_ = 1;

		Uint32 historyValid_ = 0;

		Uint32 raytracingReflectionConstantBufferPadding0_ = 0;
	};
	SC_STATIC_ASSERT(RaytracingReflectionConstantBuffer, 32, "Illumination/Reflection/Raytracing/RaytracingReflection.hlsli");

	struct RaytracingReflectionReservoir
	{
		Vector3 sampleDirection_ = { 0.0f, 0.0f, 0.0f };

		Float sampleM_ = 0.0f;

		Vector3 sampleRadiance_ = { 0.0f, 0.0f, 0.0f };

		Float sampleW_ = 0.0f;

		Vector3 receiverNormal_ = { 0.0f, 0.0f, 0.0f };

		Float receiverDepth_ = 0.0f;

		Float sampleHitDistance_ = 0.0f;

		Float sampleAge_ = 0.0f;

		Vector2 reflectionReservoirPadding_ = { 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(RaytracingReflectionReservoir, 64, "Illumination/Reflection/Raytracing/RaytracingReflectionReSTIR.hlsli");

	struct RaytracingReflectionTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionTraceRT.hlsl");

		SC_CONSTEX const Char* rayGeneration_ = "RaytracingReflectionRayGeneration";

		SC_CONSTEX const Char* miss_ = "RaytracingReflectionMiss";

		SC_CONSTEX const Char* closestHit_ = "RaytracingReflectionClosestHit";

		SC_CONSTEX const Char* anyHit_ = "RaytracingReflectionAnyHit";

		SC_CONSTEX const Char* hitGroup_ = "RaytracingReflectionHitGroup";

		SC_CONSTEX Uint32 payloadSize_ = 16;

		SC_CONSTEX Uint32 recursionDepth_ = 1;
	};

	struct RaytracingReflectionSpatialDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionSpatialCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingReflectionTemporalDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Temporal";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingReflectionMomentsDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Moments";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingReflectionATrousDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "ATrous";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
