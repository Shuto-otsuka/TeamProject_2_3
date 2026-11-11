#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct HUDOutputDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Shape/HUD/HUDOutputPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::FullscreenTriangle;

		SC_CONSTEX StaticArray<DXGI_FORMAT, 1> targetFormat_ = { DXGI_FORMAT_R8G8B8A8_UNORM };

		SC_CONSTEX DXGI_FORMAT depthFormat_ = DXGI_FORMAT_UNKNOWN;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
