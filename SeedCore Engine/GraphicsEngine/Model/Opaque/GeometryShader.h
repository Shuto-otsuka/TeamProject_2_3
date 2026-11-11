#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/Culling/CullingFeature.h>

namespace SeedCore
{
	struct GeometryDepthDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryPS.hlsl");

		SC_CONSTEX const Char* pixelEntryPoint_ = "Depth";

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::DrawArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::DrawArguments;

		SC_CONSTEX StaticArray<DXGI_FORMAT, 0> targetFormat_ = {};

		SC_CONSTEX DXGI_FORMAT depthFormat_ = DXGI_FORMAT_D32_FLOAT;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOnReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct GeometryVisibilityDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryPS.hlsl");

		SC_CONSTEX const Char* pixelEntryPoint_ = "Visibility";

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Indirect;

		SC_CONSTEX RenderResource arguments_ = CullingFeature::DrawArguments;

		SC_CONSTEX RenderResource fallbackArguments_ = CullingFeature::DrawArguments;

		SC_CONSTEX StaticArray<DXGI_FORMAT, 2> targetFormat_ = { DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_FLOAT };

		SC_CONSTEX DXGI_FORMAT depthFormat_ = DXGI_FORMAT_D32_FLOAT;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Opaque;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOffReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct GeometryResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/Opaque/GeometryCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Resolve";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}
