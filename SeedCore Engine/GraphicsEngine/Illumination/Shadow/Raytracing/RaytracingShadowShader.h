#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct RaytracingShadowConstantBuffer
	{
		Float rayTMax_ = 1000.0f;

		Float normalBias_ = 0.01f;

		Float strength_ = 1.0f;

		Float sunAngularRadius_ = 0.02f;

		Float punctualRadius_ = 0.1f;

		Uint32 rayCount_ = 1;

		Uint32 frame_ = 0;

		Uint32 historyValid_ = 0;
	};
	SC_STATIC_ASSERT(RaytracingShadowConstantBuffer, 32, "Illumination/Shadow/Raytracing/RaytracingShadow.hlsli");

	struct RaytracingShadowTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowTraceCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingShadowTemporalDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Temporal";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingShadowMomentsDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Moments";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct RaytracingShadowATrousDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowDenoiseCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "ATrous";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
