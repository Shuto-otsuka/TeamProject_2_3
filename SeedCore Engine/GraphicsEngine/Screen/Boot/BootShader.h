#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct BootConstantBuffer
	{
		Vector2 screenSize_ = { 1.0f, 1.0f };

		Float alpha_ = 1.0f;

		Float progress_ = 0.0f;

		Float time_ = 0.0f;

		Float backgroundAspect_ = 1.0f;

		Uint fillMethod_ = 0;

		Uint fillOrigin_ = 0;

		Uint clockwise_ = 0;

		Uint wave_ = 0;

		Uint useBackgroundImage_ = 0;

		Uint useFrame_ = 0;

		Vector4 barRect_ = { 0.0f, 0.0f, 0.0f, 0.0f };

		Vector4 backgroundColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

		Vector4 backgroundTint_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		Vector4 barTint_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		Vector4 frameTint_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		Uint backgroundTexture_ = SC_INVALID;

		Uint barTexture_ = SC_INVALID;

		Uint frameTexture_ = SC_INVALID;

		Uint bootConstantBufferPadding0_ = 0;
	};
	SC_STATIC_ASSERT(BootConstantBuffer, 144, "Screen/Boot/BootPS.hlsl");

	struct BootDrawDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Screen/Boot/BootPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::FullscreenTriangle;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA8Unorm };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Alpha;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
