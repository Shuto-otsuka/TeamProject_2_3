#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct FilmGrainConstantBuffer
	{
		Uint32 colored_ = 0;

		Float intensity_ = 0.0f;

		Float size_ = 0.0f;

		Float luminanceResponse_ = 0.0f;
	};
	SC_STATIC_ASSERT(FilmGrainConstantBuffer, 16, "PostProcess/FilmGrain/FilmGrainCS.hlsl");

	struct FilmGrainResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/FilmGrain/FilmGrainCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
