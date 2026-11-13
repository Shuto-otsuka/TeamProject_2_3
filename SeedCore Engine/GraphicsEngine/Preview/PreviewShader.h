#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct PreviewLineStructuredBuffer
	{
		Vector3 start_ = { 0.0f, 0.0f, 0.0f };

		Float width_ = 2.0f;

		Vector3 end_ = { 0.0f, 0.0f, 0.0f };

		Float previewLineStructuredBufferPadding0_ = 0.0f;

		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
	SC_STATIC_ASSERT(PreviewLineStructuredBuffer, 48, "Preview/Preview.hlsli");

	struct PreviewMeshDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Preview/PreviewMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Preview/PreviewVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Preview/PreviewPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 1;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 372;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA8Unorm };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOnReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct PreviewLineDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Preview/PreviewLineMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Preview/PreviewLineVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Preview/PreviewLinePS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 32;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 6;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA8Unorm };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Alpha;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}
