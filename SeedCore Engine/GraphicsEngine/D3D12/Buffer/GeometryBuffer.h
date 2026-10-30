#pragma once
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12CommandList;

	class GeometryBuffer
	{
	private:
		SC_CONSTEX Int bufferCount_ = 5;

		const DXGI_FORMAT formats_[bufferCount_] =
		{
			DXGI_FORMAT_R8G8B8A8_UNORM,		// RT0: base_color.rgb + metallic
			DXGI_FORMAT_R16G16B16A16_UNORM,	// RT1: octNormal.rg + roughness (.a unused/reserved - KHR extension scalars are no longer baked per-pixel, DeferredLightingPS.hlsl reads them straight from ModelStructuredBuffer via VisID)
			DXGI_FORMAT_R16G16_FLOAT,		// RT2: velocity
			DXGI_FORMAT_R11G11B10_FLOAT,	// RT3: emissive.rgb (raw texture*factor - emissive_strength_ is applied at lighting time, not baked in)
			DXGI_FORMAT_R32G32B32A32_UINT	// RT4: visibility id (instance_index, pack(meshlet_index, triangle_in_meshlet_index)) + asuint(texcoord) - VisibilityBuffer groundwork
		};

	public:
		GeometryBuffer() = default;
		~GeometryBuffer() = default;

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, Uint32 width, Uint32 height);

		void Destroy(BindlessHeap* bindlessHeap);

		void Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, Uint32 width, Uint32 height);

		void Begin(D3D12CommandList* cmdList);

		void BeginDepthOnly(D3D12CommandList* cmdList);

		void BeginVisibility(D3D12CommandList* cmdList);

		void BeginDepth(D3D12CommandList* cmdList);

		void Clear(D3D12CommandList* cmdList);

		void ClearDepth(D3D12CommandList* cmdList);

		void ClearVisibility(D3D12CommandList* cmdList);

		void EndColor(D3D12CommandList* cmdList);

		void EndDepthNonPixel(D3D12CommandList* cmdList);

		void EndDepth(D3D12CommandList* cmdList);

		void End(D3D12CommandList* cmdList);

		NODISCARD Uint32 ColorShaderResourceViewIndex(Int index)const;

		NODISCARD Uint32 DepthShaderResourceViewIndex()const;

		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilViewHandle()const;

		NODISCARD ID3D12Resource* ColorResource(Int index)const;

		NODISCARD ID3D12Resource* DepthResource()const;

		NODISCARD Uint32 ColorUnorderedAccessViewIndex(Int index)const;

		NODISCARD Uint32 VelocityUnorderedAccessViewIndex()const;

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> colorResources_[bufferCount_];
		D3D12_RESOURCE_STATES colorStates_[bufferCount_] = {};

		Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_;
		D3D12_RESOURCE_STATES depthState_ = D3D12_RESOURCE_STATE_COMMON;

		DescriptorHeap renderTargetViewHeap_;
		DescriptorHeap depthStencilViewHeap_;

		Uint32 colorShaderResourceViewIndices_[bufferCount_] = {};
		Uint32 depthShaderResourceViewIndex_ = 0;
		Uint32 colorUnorderedAccessViewIndices_[bufferCount_] = {};

		D3D12_VIEWPORT viewport_;
	};
}
