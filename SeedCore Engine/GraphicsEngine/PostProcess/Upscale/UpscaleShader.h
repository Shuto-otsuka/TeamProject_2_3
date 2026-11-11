#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct UpscaleTaauResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Upscale/UpscaleTaauCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct UpscalePrepareGuidesDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Upscale/UpscaleGuidesCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
