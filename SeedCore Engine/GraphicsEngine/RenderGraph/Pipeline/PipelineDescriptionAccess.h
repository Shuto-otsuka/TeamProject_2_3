#pragma once
#include <GraphicsEngine/Shader/ShaderPath.h>
#include <GraphicsEngine/RenderGraph/RenderResource.h>
#include <GraphicsEngine/D3D12/PipelineState/BlendState.h>
#include <GraphicsEngine/D3D12/PipelineState/DepthStencilState.h>
#include <GraphicsEngine/D3D12/PipelineState/RasterizerState.h>

namespace SeedCore
{
	enum class GraphicsFallback :Uint8
	{
		Instanced,
		Indirect,
		FullscreenTriangle,
	};

	class PipelineDescriptionAccess
	{
	public:
		template<class Description>
		SC_CONSTEX Bool IsComputeDescription = requires
		{
			{ Description::file_ } -> std::convertible_to<ShaderPath>;
			{ Description::entryPoint_ } -> std::convertible_to<const Char*>;
			{ Description::threadGroupSizeX_ } -> std::convertible_to<Uint32>;
			{ Description::threadGroupSizeY_ } -> std::convertible_to<Uint32>;
		};

		template<class Description>
		SC_CONSTEX Bool IsGraphicsDescription = requires
		{
			{ Description::meshFile_ } -> std::convertible_to<ShaderPath>;
			{ Description::vertexFile_ } -> std::convertible_to<ShaderPath>;
			{ Description::fallback_ } -> std::convertible_to<GraphicsFallback>;
			{ Description::targetFormat_.size() } -> std::convertible_to<Size>;
			{ Description::depthFormat_ } -> std::convertible_to<DXGI_FORMAT>;
			{ Description::blend_ } -> std::convertible_to<BlendStateType>;
			{ Description::depthStencil_ } -> std::convertible_to<DepthStencilStateType>;
			{ Description::rasterizer_ } -> std::convertible_to<RasterizerStateType>;
		};

		template<class Description>
		SC_CONSTEX Bool IsInstancedDescription = requires
		{
			{ Description::instancesPerGroup_ } -> std::convertible_to<Uint32>;
			{ Description::fallbackVertexCount_ } -> std::convertible_to<Uint32>;
		};

		template<class Description>
		SC_CONSTEX Bool IsIndirectDescription = requires
		{
			{ Description::arguments_ } -> std::convertible_to<RenderResource>;
			{ Description::fallbackArguments_ } -> std::convertible_to<RenderResource>;
		};

		template<class Description>
		SC_CONSTEX Bool IsRaytracingDescription = requires
		{
			{ Description::file_ } -> std::convertible_to<ShaderPath>;
			{ Description::rayGeneration_ } -> std::convertible_to<const Char*>;
			{ Description::miss_ } -> std::convertible_to<const Char*>;
			{ Description::closestHit_ } -> std::convertible_to<const Char*>;
			{ Description::hitGroup_ } -> std::convertible_to<const Char*>;
			{ Description::payloadSize_ } -> std::convertible_to<Uint32>;
			{ Description::recursionDepth_ } -> std::convertible_to<Uint32>;
		};
	};
}