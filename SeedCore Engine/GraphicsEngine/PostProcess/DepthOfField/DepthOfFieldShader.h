#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct DepthOfFieldConstantBuffer
	{
		Float focusDistance_ = 0.0f;

		Float focusRange_ = 0.0f;

		Float maxBlurRadius_ = 0.0f;

		Float padding_ = 0.0f;
	};
	SC_STATIC_ASSERT(DepthOfFieldConstantBuffer, 16, "PostProcess/DepthOfField/DepthOfFieldCS.hlsl");

	struct DepthOfFieldBlurDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/DepthOfField/DepthOfFieldCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
