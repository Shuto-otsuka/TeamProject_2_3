#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct ToneMappingConstantBuffer
	{
		Uint32 enabled_ = 0;

		Uint32 mode_ = 0;

		StaticArray<Float, 2> padding_ = { 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(ToneMappingConstantBuffer, 16, "PostProcess/ToneMapping/ToneMappingCS.hlsl");

	struct ToneMappingResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/ToneMapping/ToneMappingCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
