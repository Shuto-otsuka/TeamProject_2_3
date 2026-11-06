#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/Shader/ShaderHotReload.h>

namespace SeedCore
{
	RenderDevice::RenderDevice() = default;

	RenderDevice::~RenderDevice() = default;

	Bool RenderDevice::Initialize()
	{
		context_ = MakePtr<D3D12Context>();
		if (!context_->Initialize())
		{
			return false;
		}
		level_ = D3D12Check::GetLevel();

		shaderCache_ = MakePtr<SeedCore::ShaderCache>();
		shaderHotReload_ = MakePtr<SeedCore::ShaderHotReload>();

		resourceHeap_ = MakePtr<BindlessHeap>();
		if (!resourceHeap_->Create(context_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536))
		{
			return false;
		}

		samplerHeap_ = MakePtr<BindlessHeap>();
		if (!samplerHeap_->Create(context_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE))
		{
			return false;
		}

		renderTargetHeap_ = MakePtr<BindlessHeap>();
		if (!renderTargetHeap_->Create(context_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024, false))
		{
			return false;
		}

		depthStencilHeap_ = MakePtr<BindlessHeap>();
		if (!depthStencilHeap_->Create(context_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256, false))
		{
			return false;
		}

		rootSignature_.Create(context_->GetDevice());
		return true;
	}

	D3D12Context& RenderDevice::Context()const
	{
		return *context_;
	}

	SeedCore::ShaderCache& RenderDevice::ShaderCache()const
	{
		return *shaderCache_;
	}

	SeedCore::ShaderHotReload& RenderDevice::ShaderHotReload()const
	{
		return *shaderHotReload_;
	}

	BindlessHeap& RenderDevice::ResourceHeap()const
	{
		return *resourceHeap_;
	}

	BindlessHeap& RenderDevice::SamplerHeap()const
	{
		return *samplerHeap_;
	}

	BindlessHeap& RenderDevice::RenderTargetHeap()const
	{
		return *renderTargetHeap_;
	}

	BindlessHeap& RenderDevice::DepthStencilHeap()const
	{
		return *depthStencilHeap_;
	}

	const SeedCore::RootSignature& RenderDevice::RootSignature()const
	{
		return rootSignature_;
	}

	D3D12Level RenderDevice::Level()const
	{
		return level_;
	}
}