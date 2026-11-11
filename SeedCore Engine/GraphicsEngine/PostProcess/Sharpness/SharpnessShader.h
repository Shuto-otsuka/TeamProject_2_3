#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct SharpnessConstantBuffer
	{
		Float amount_ = 0.0f;

		StaticArray<Float, 3> padding_ = { 0.0f, 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(SharpnessConstantBuffer, 16, "PostProcess/Sharpness/SharpnessCS.hlsl");

	struct SharpnessResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Sharpness/SharpnessCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
