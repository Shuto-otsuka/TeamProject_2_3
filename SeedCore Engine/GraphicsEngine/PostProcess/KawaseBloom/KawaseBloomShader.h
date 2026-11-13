#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct KawaseBloomConstantBuffer
	{
		Float threshold_ = 0.0f;

		Float softKnee_ = 0.0f;

		Float filterRadius_ = 0.0f;

		Float padding_ = 0.0f;
	};
	SC_STATIC_ASSERT(KawaseBloomConstantBuffer, 16, "PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

	struct KawaseBloomPrefilterDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Prefilter";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomDownsampleDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomUpsampleDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Upsample";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
