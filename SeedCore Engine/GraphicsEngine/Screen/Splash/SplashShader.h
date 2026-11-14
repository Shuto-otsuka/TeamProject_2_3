#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct SplashConstantBuffer
	{
		Vector2 screenSize_ = { 1.0f, 1.0f };

		Float alpha_ = 1.0f;

		Float aspect_ = 1.0f;

		Uint texture_ = SC_INVALID;

		Uint splashConstantBufferPadding0_ = 0;

		Uint splashConstantBufferPadding1_ = 0;

		Uint splashConstantBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(SplashConstantBuffer, 32, "Screen/Splash/SplashPS.hlsl");

	struct SplashDrawDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Fullscreen/FullscreenVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Screen/Splash/SplashPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::FullscreenTriangle;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA8Unorm };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
