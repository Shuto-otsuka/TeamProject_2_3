#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct VignetteConstantBuffer
	{
		Float intensity_ = 0.0f;

		Float exponent_ = 0.0f;

		StaticArray<Float, 2> padding_ = { 0.0f, 0.0f };

		StaticArray<Float, 4> color_ = { 0.0f, 0.0f, 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(VignetteConstantBuffer, 32, "PostProcess/Vignette/VignetteCS.hlsl");

	struct VignetteResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Vignette/VignetteCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
