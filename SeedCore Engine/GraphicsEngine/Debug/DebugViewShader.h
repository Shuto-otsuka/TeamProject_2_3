#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>
#include <GraphicsEngine/Culling/CullingFeature.h>

namespace SeedCore
{
	struct DebugViewClearDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Debug/DebugViewCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Clear";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct DebugViewWireframeDescription
	{
		SC_CONSTEX ShaderPath amplificationFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryAS.hlsl");

		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Debug/DebugViewPS.hlsl");

		SC_CONSTEX const Char* pixelEntryPoint_ = "Wireframe";

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::ChunkArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::MeshletArguments;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::WireNoneLHS;
	};

	struct DebugViewOverdrawDescription
	{
		SC_CONSTEX ShaderPath amplificationFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryAS.hlsl");

		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Debug/DebugViewPS.hlsl");

		SC_CONSTEX const Char* pixelEntryPoint_ = "Overdraw";

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::ChunkArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::MeshletArguments;

		SC_CONSTEX StaticArray<RenderFormat, 0> targetFormat_ = {};

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct DebugViewResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Debug/DebugViewCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Resolve";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct DebugViewBoundsDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Debug/DebugViewCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Bounds";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}
