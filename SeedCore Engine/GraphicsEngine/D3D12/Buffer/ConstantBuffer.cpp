#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	DynamicConstantBuffer::DynamicConstantBuffer(ID3D12Device* device, BindlessHeap* heap, Uint byteSize) : heap_(heap)
	{
		Allocate(device, byteSize);
	}

	DynamicConstantBuffer::~DynamicConstantBuffer()
	{
		Release();
	}

	void DynamicConstantBuffer::Update(ID3D12Device* device, const void* data, Uint byteSize)
	{
		if (byteSize > capacity_)
		{
			Release();
			Allocate(device, byteSize);
			if (byteSize > capacity_)
			{
				return;
			}
		}
		memcpy(mappedPtrs_[FrameRing::Index()], data, byteSize);
	}

	D3D12_GPU_VIRTUAL_ADDRESS DynamicConstantBuffer::Address()const
	{
		const Microsoft::WRL::ComPtr<ID3D12Resource>& resource = resources_[FrameRing::Index()];
		if (!resource)
		{
			return 0;
		}
		return resource->GetGPUVirtualAddress();
	}

	Uint DynamicConstantBuffer::Index()const
	{
		return indices_[FrameRing::Index()];
	}

	void DynamicConstantBuffer::Allocate(ID3D12Device* device, Uint byteSize)
	{
		if (byteSize == 0 || byteSize > 65536)
		{
			SC_LOG_ERROR("可変定数バッファ: サイズが範囲外です: {}", byteSize);
			for (Uint frame = 0; frame < FrameRing::frameCount; frame++)
			{
				indices_[frame] = SC_INVALID;
			}
			return;
		}

		Uint64 alignedSize = (byteSize + 255) & ~255ull;
		capacity_ = static_cast<Uint>(alignedSize);

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = alignedSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		for (Uint frame = 0; frame < FrameRing::frameCount; frame++)
		{
			HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resources_[frame]));
			SC_HR_CHECK(hr, "定数バッファの生成に失敗しました");
#ifdef _DEBUG
			resources_[frame]->SetName(L"DynamicConstantBuffer");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(resources_[frame].Get());
#endif

			indices_[frame] = heap_->AllocateIndex();
			D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_->CPUHandle(indices_[frame]);
			if (handle.ptr != 0)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc{};
				constantBufferViewDesc.BufferLocation = resources_[frame]->GetGPUVirtualAddress();
				constantBufferViewDesc.SizeInBytes = static_cast<Uint>(alignedSize);
				device->CreateConstantBufferView(&constantBufferViewDesc, handle);
			}

			resources_[frame]->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtrs_[frame]));
		}
	}

	void DynamicConstantBuffer::Release()
	{
		for (Uint frame = 0; frame < FrameRing::frameCount; frame++)
		{
			if (resources_[frame])
			{
				resources_[frame]->Unmap(0, nullptr);
				heap_->Release(std::move(resources_[frame]), indices_[frame]);
			}
			mappedPtrs_[frame] = nullptr;
			indices_[frame] = SC_INVALID;
		}
		capacity_ = 0;
	}
}