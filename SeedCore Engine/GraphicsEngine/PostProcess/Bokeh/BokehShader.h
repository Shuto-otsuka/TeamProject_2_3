#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct BokehConstantBuffer
	{
		Float focusDistance_ = 0.0f;

		Float focusRange_ = 0.0f;

		Float maxBlurRadius_ = 0.0f;

		Float highlightThreshold_ = 0.0f;

		Float highlightIntensity_ = 0.0f;

		Uint32 bladeCount_ = 0;

		StaticArray<Float, 2> padding_ = { 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(BokehConstantBuffer, 32, "PostProcess/Bokeh/BokehCS.hlsl");

	struct BokehResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Bokeh/BokehCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
