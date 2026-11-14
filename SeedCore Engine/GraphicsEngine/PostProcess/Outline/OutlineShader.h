#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct OutlineStyleStructuredBuffer
	{
		Uint color_ = 0;

		Float width_ = 0.0f;

		Uint flags_ = 0;

		Uint outlineStyleStructuredBufferPadding0_ = 0;
	};
	SC_STATIC_ASSERT(OutlineStyleStructuredBuffer, 16, "PostProcess/Outline/Outline.hlsli");

	struct OutlineStyleUpdate
	{
		Uint index_ = 0;

		Uint outlineStyleUpdatePadding0_ = 0;

		Uint outlineStyleUpdatePadding1_ = 0;

		Uint outlineStyleUpdatePadding2_ = 0;

		OutlineStyleStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(OutlineStyleUpdate, 32, "PostProcess/Outline/Outline.hlsli");

	struct OutlineStyleDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Outline/OutlineCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Style";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct OutlineMaskDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Outline/OutlineCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Mask";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct OutlineResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Outline/OutlineCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Resolve";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct OutlineCanvasDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Outline/OutlineCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Canvas";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct OutlineComposeDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/PostProcess/Outline/OutlinePS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::FullscreenTriangle;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Alpha;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
