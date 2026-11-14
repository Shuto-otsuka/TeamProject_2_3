#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct HUDComposeConstantBuffer
	{
		Vector4 outputRect_ = { 0.0f, 0.0f, 1.0f, 1.0f };

		Float fade_ = 0.0f;

		Vector3 background_ = { 0.0f, 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(HUDComposeConstantBuffer, 32, "Shape/HUD/HUDOutputPS.hlsl");

	struct HUDComposeDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Shape/HUD/HUDOutputPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::FullscreenTriangle;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA8Unorm };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
