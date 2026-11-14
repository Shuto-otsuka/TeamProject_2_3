#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct ColorGradingConstantBuffer
	{
		Float shadowsMax_ = 0.0f;

		Float highlightsMin_ = 0.0f;

		StaticArray<Float, 2> padding_ = { 0.0f, 0.0f };

		StaticArray<Float, 32> ranges_ = {};
	};
	SC_STATIC_ASSERT(ColorGradingConstantBuffer, 144, "PostProcess/ColorGrading/ColorGradingCS.hlsl");

	struct ColorGradingResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/ColorGrading/ColorGradingCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
