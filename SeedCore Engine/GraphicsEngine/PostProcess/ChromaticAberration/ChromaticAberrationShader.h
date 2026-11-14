#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct ChromaticAberrationConstantBuffer
	{
		Float intensity_ = 0.0f;

		Uint32 sampleCount_ = 0;

		StaticArray<Float, 2> padding_ = { 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(ChromaticAberrationConstantBuffer, 16, "PostProcess/ChromaticAberration/ChromaticAberrationCS.hlsl");

	struct ChromaticAberrationResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
