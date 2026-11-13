#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct DeferredLightingResolveDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/DeferredLightingPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::FullscreenTriangle;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Additive;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
