#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct LensDistortionConstantBuffer
	{
		Float k1_ = 0.0f;

		Float k2_ = 0.0f;

		Float k3_ = 0.0f;

		Float scale_ = 1.0f;
	};
	SC_STATIC_ASSERT(LensDistortionConstantBuffer, 16, "PostProcess/LensDistortion/LensDistortionCS.hlsl");

	struct LensDistortionResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensDistortion/LensDistortionCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
