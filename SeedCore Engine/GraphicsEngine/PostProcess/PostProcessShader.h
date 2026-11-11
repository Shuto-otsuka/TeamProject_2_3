#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct PostProcessComposeConstantBuffer
	{
		Float exposureCompensation_ = 0.0f;

		Uint32 autoExposure_ = 0;

		Float bloomIntensity_ = 0.0f;

		Float padding_ = 0.0f;
	};
	SC_STATIC_ASSERT(PostProcessComposeConstantBuffer, 16, "PostProcess/PostProcessCS.hlsl");

	struct PostProcessComposeDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/PostProcessCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Compose";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
