#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>
#include <GraphicsEngine/Culling/CullingFeature.h>

namespace SeedCore
{
	struct TransparentFragmentStructuredBuffer
	{
		Uint colorRG_ = 0;

		Uint colorBA_ = 0;

		Float depth_ = 0.0f;

		Uint next_ = SC_INVALID;

		Uint velocity_ = 0;
	};
	SC_STATIC_ASSERT(TransparentFragmentStructuredBuffer, 20, "Model/Transparent/Transparent.hlsli");

	struct TransparentClearDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/Transparent/TransparentCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Clear";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct TransparentResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/Transparent/TransparentCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Resolve";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct TransparentDrawDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Transparent/TransparentMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Transparent/TransparentVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Model/Transparent/TransparentPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::TransparentArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::TransparentArguments;

		SC_CONSTEX StaticArray<RenderFormat, 0> targetFormat_ = {};

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOffReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
