#pragma once
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12Context;
	class ShaderCache;
	class ShaderHotReload;

	class RenderDevice :public NonTransferable
	{
	public:
		RenderDevice();

		~RenderDevice();

		Bool Initialize();

	public:
		NODISCARD D3D12Context& Context()const;

		NODISCARD SeedCore::ShaderCache& ShaderCache()const;

		NODISCARD SeedCore::ShaderHotReload& ShaderHotReload()const;

		NODISCARD BindlessHeap& ResourceHeap()const;

		NODISCARD BindlessHeap& SamplerHeap()const;

		NODISCARD BindlessHeap& RenderTargetHeap()const;

		NODISCARD BindlessHeap& DepthStencilHeap()const;

		NODISCARD const SeedCore::RootSignature& RootSignature()const;

		NODISCARD D3D12Level Level()const;

	private:
		ResourcePtr<D3D12Context> context_;

		ResourcePtr<SeedCore::ShaderCache> shaderCache_;

		ResourcePtr<SeedCore::ShaderHotReload> shaderHotReload_;

		ResourcePtr<BindlessHeap> resourceHeap_;

		ResourcePtr<BindlessHeap> samplerHeap_;

		ResourcePtr<BindlessHeap> renderTargetHeap_;

		ResourcePtr<BindlessHeap> depthStencilHeap_;

		SeedCore::RootSignature rootSignature_;

		D3D12Level level_ = D3D12Level::D12_0;
	};
}