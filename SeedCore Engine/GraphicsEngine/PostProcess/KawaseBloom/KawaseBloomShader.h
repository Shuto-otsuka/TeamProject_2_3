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

		SC_CONSTEX const Char* entryPoint_ = "DownsamplePrefilter";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomDownsample1Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample1";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomDownsample2Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample2";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomDownsample3Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample3";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomDownsample4Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample4";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomDownsample5Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample5";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomUpsample4Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Upsample4";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomUpsample3Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Upsample3";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomUpsample2Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Upsample2";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomUpsample1Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Upsample1";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct KawaseBloomUpsample0Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Upsample0";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
