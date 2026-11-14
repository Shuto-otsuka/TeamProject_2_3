#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/Culling/CullingFeature.h>

namespace SeedCore
{
	struct GeometryDepthDescription
	{
		SC_CONSTEX ShaderPath amplificationFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryAS.hlsl");

		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryPS.hlsl");

		SC_CONSTEX const Char* pixelEntryPoint_ = "Depth";

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::ChunkArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::MeshletArguments;

		SC_CONSTEX StaticArray<RenderFormat, 0> targetFormat_ = {};

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOnReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct GeometryVisibilityDescription
	{
		SC_CONSTEX ShaderPath amplificationFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryAS.hlsl");

		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryPS.hlsl");

		SC_CONSTEX const Char* pixelEntryPoint_ = "Visibility";

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::ChunkArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::MeshletArguments;

		SC_CONSTEX StaticArray<RenderFormat, 2> targetFormat_ = { RenderFormat::RG32Uint, RenderFormat::RG32Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOffReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct GeometryUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct GeometryPyramidDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Pyramid";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
