#include <GraphicsEngine/RenderGraph/RenderGraphMemory.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <FoundationEngine/Log/DxFail.h>

namespace SeedCore
{
	RenderGraphAllocation RenderGraphMemory::Create(RenderDevice& device, const RenderGraphResource& resource, const RenderGraphInstance& instance, const Color& clear)
	{
		const RenderResourceDesc& desc = resource.desc_;
		Uint32 mipLimit = 1;
		while ((Max(instance.width_, instance.height_) >> mipLimit) > 0)
		{
			++mipLimit;
		}
		RenderGraphAllocation allocation;
		allocation.shape_ = desc.shape_;
		allocation.format_ = desc.format_;
		allocation.width_ = instance.width_;
		allocation.height_ = instance.height_;
		allocation.depth_ = instance.depth_;
		allocation.mipLevels_ = Min(desc.mipLevels_, mipLimit);
		allocation.stride_ = desc.stride_;
		allocation.count_ = instance.count_;
		allocation.shaderResource_ = resource.shaderResource_;
		allocation.unorderedAccess_ = resource.unorderedAccess_;
		allocation.renderTarget_ = resource.renderTarget_;
		allocation.depthStencil_ = resource.depthStencil_;

		if (desc.shape_ == RenderResourceShape::AccelerationStructure)
		{
			Microsoft::WRL::ComPtr<ID3D12Device5> raytracingDevice;
			HRESULT hr = device.Context().GetDevice()->QueryInterface(IID_PPV_ARGS(&raytracingDevice));
			SC_HR_CHECK(hr, "RenderGraph: ID3D12Device5 を取得できません");

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
			inputs.NumDescs = instance.count_;

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
			raytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = Max(prebuildInfo.ResultDataMaxSizeInBytes, 1ull);
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			D3D12_HEAP_PROPERTIES heapProperties{};
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

			allocation.state_ = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			hr = raytracingDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, allocation.state_, nullptr, IID_PPV_ARGS(&allocation.resource_));
			SC_HR_CHECK(hr, "RenderGraph の AccelerationStructure の生成に失敗しました");
			allocation.resource_->SetName(std::wstring(resource.resourceName_.w_str()).c_str());

			if (resource.shaderResource_)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
				shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
				shaderResourceViewDesc.RaytracingAccelerationStructure.Location = allocation.resource_->GetGPUVirtualAddress();
				allocation.shaderResourceView_ = device.ResourceHeap().AllocateIndex();
				raytracingDevice->CreateShaderResourceView(nullptr, &shaderResourceViewDesc, device.ResourceHeap().CPUHandle(allocation.shaderResourceView_));
			}
			return allocation;
		}

		Bool buffer = desc.shape_ == RenderResourceShape::Buffer;
		Bool depthFormat = desc.format_ == RenderFormat::D32Float;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = buffer ? D3D12_RESOURCE_DIMENSION_BUFFER : desc.shape_ == RenderResourceShape::Texture3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = buffer ? static_cast<Uint64>(Max(desc.stride_ * instance.count_, 1u)) : instance.width_;
		resourceDesc.Height = buffer ? 1 : instance.height_;
		resourceDesc.DepthOrArraySize = buffer ? 1 : static_cast<Uint16>(instance.depth_);
		resourceDesc.MipLevels = buffer ? 1 : static_cast<Uint16>(allocation.mipLevels_);
		resourceDesc.Format = buffer ? DXGI_FORMAT_UNKNOWN : depthFormat ? DXGI_FORMAT_R32_TYPELESS : static_cast<DXGI_FORMAT>(desc.format_);
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = buffer ? D3D12_TEXTURE_LAYOUT_ROW_MAJOR : D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (resource.unorderedAccess_)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (resource.renderTarget_)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (resource.depthStencil_)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_CLEAR_VALUE clearValue{};
		const D3D12_CLEAR_VALUE* optimizedClearValue = nullptr;
		if (resource.depthStencil_)
		{
			clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			clearValue.DepthStencil.Depth = 0.0f;
			optimizedClearValue = &clearValue;
		}
		else if (resource.renderTarget_)
		{
			clearValue.Format = static_cast<DXGI_FORMAT>(desc.format_);
			clearValue.Color[0] = clear.x;
			clearValue.Color[1] = clear.y;
			clearValue.Color[2] = clear.z;
			clearValue.Color[3] = clear.w;
			optimizedClearValue = &clearValue;
		}

		ID3D12Device* d3dDevice = device.Context().GetDevice();
		HRESULT hr = d3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, optimizedClearValue, IID_PPV_ARGS(&allocation.resource_));
		SC_HR_CHECK(hr, "RenderGraph のリソースの生成に失敗しました");
		allocation.resource_->SetName(std::wstring(resource.resourceName_.w_str()).c_str());

		if (resource.shaderResource_)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Format = buffer ? DXGI_FORMAT_UNKNOWN : depthFormat ? DXGI_FORMAT_R32_FLOAT : static_cast<DXGI_FORMAT>(desc.format_);
			if (buffer)
			{
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				shaderResourceViewDesc.Buffer.NumElements = instance.count_;
				shaderResourceViewDesc.Buffer.StructureByteStride = desc.stride_;
			}
			else if (desc.shape_ == RenderResourceShape::Texture2D)
			{
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Texture2D.MipLevels = UINT_MAX;
			}
			else if (desc.shape_ == RenderResourceShape::Texture2DArray)
			{
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				shaderResourceViewDesc.Texture2DArray.MipLevels = UINT_MAX;
				shaderResourceViewDesc.Texture2DArray.ArraySize = instance.depth_;
			}
			else if (desc.shape_ == RenderResourceShape::TextureCube)
			{
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				shaderResourceViewDesc.TextureCube.MipLevels = UINT_MAX;
			}
			else if (desc.shape_ == RenderResourceShape::TextureCubeArray)
			{
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
				shaderResourceViewDesc.TextureCubeArray.MipLevels = UINT_MAX;
				shaderResourceViewDesc.TextureCubeArray.NumCubes = instance.depth_ / 6;
			}
			else
			{
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				shaderResourceViewDesc.Texture3D.MipLevels = UINT_MAX;
			}
			allocation.shaderResourceView_ = device.ResourceHeap().AllocateIndex();
			d3dDevice->CreateShaderResourceView(allocation.resource_.Get(), &shaderResourceViewDesc, device.ResourceHeap().CPUHandle(allocation.shaderResourceView_));
		}

		if (resource.unorderedAccess_)
		{
			Uint32 mipLevels = buffer ? 1 : allocation.mipLevels_;
			allocation.unorderedAccessView_ = mipLevels > 1 ? device.ResourceHeap().AllocateRange(mipLevels) : device.ResourceHeap().AllocateIndex();
			for (Uint32 mip = 0;mip < mipLevels;++mip)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC unorderdAccessViewDesc{};
				unorderdAccessViewDesc.Format = buffer ? DXGI_FORMAT_UNKNOWN : static_cast<DXGI_FORMAT>(desc.format_);
				if (buffer)
				{
					unorderdAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					unorderdAccessViewDesc.Buffer.NumElements = instance.count_;
					unorderdAccessViewDesc.Buffer.StructureByteStride = desc.stride_;
				}
				else if (desc.shape_ == RenderResourceShape::Texture3D)
				{
					unorderdAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
					unorderdAccessViewDesc.Texture3D.MipSlice = mip;
					unorderdAccessViewDesc.Texture3D.WSize = Max(instance.depth_ >> mip, 1u);
				}
				else if (desc.shape_ == RenderResourceShape::Texture2D)
				{
					unorderdAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					unorderdAccessViewDesc.Texture2D.MipSlice = mip;
				}
				else
				{
					unorderdAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					unorderdAccessViewDesc.Texture2DArray.MipSlice = mip;
					unorderdAccessViewDesc.Texture2DArray.ArraySize = instance.depth_;
				}
				d3dDevice->CreateUnorderedAccessView(allocation.resource_.Get(), nullptr, &unorderdAccessViewDesc, device.ResourceHeap().CPUHandle(allocation.unorderedAccessView_ + mip));
			}
		}

		if (resource.renderTarget_)
		{
			D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
			renderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(desc.format_);
			renderTargetViewDesc.ViewDimension = desc.shape_ == RenderResourceShape::Texture2D ? D3D12_RTV_DIMENSION_TEXTURE2D : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			renderTargetViewDesc.Texture2DArray.ArraySize = instance.depth_;
			allocation.renderTargetView_ = device.RenderTargetHeap().AllocateIndex();
			d3dDevice->CreateRenderTargetView(allocation.resource_.Get(), &renderTargetViewDesc, device.RenderTargetHeap().CPUHandle(allocation.renderTargetView_));
		}

		if (resource.depthStencil_)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
			depthStencilViewDesc.Format = static_cast<DXGI_FORMAT>(desc.format_);
			depthStencilViewDesc.ViewDimension = desc.shape_ == RenderResourceShape::Texture2D ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			depthStencilViewDesc.Texture2DArray.ArraySize = instance.depth_;
			allocation.depthStencilView_ = device.DepthStencilHeap().AllocateIndex();
			d3dDevice->CreateDepthStencilView(allocation.resource_.Get(), &depthStencilViewDesc, device.DepthStencilHeap().CPUHandle(allocation.depthStencilView_));

			depthStencilViewDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
			allocation.depthReadOnlyView_ = device.DepthStencilHeap().AllocateIndex();
			d3dDevice->CreateDepthStencilView(allocation.resource_.Get(), &depthStencilViewDesc, device.DepthStencilHeap().CPUHandle(allocation.depthReadOnlyView_));
		}

		return allocation;
	}

	RenderGraphAllocation RenderGraphMemory::Create(RenderDevice& device, const RenderGraphResource& resource, const StagingBuffer& buffer, const RenderUploadEntry& entry)
	{
		const RenderResourceDesc& desc = resource.desc_;
		Bool constant = desc.shape_ == RenderResourceShape::Constant;
		RenderGraphAllocation allocation;
		allocation.resource_ = buffer.Resource();
		allocation.state_ = D3D12_RESOURCE_STATE_GENERIC_READ;
		allocation.shape_ = desc.shape_;
		allocation.stride_ = desc.stride_;
		allocation.count_ = constant ? 1 : static_cast<Uint32>(entry.size_ / desc.stride_);
		allocation.shaderResource_ = true;

		ID3D12Device* d3dDevice = device.Context().GetDevice();
		allocation.shaderResourceView_ = device.ResourceHeap().AllocateIndex();
		if (constant)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc{};
			constantBufferViewDesc.BufferLocation = buffer.Address() + entry.offset_;
			constantBufferViewDesc.SizeInBytes = static_cast<Uint32>((entry.size_ + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			d3dDevice->CreateConstantBufferView(&constantBufferViewDesc, device.ResourceHeap().CPUHandle(allocation.shaderResourceView_));
			return allocation;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shaderResourceViewDesc.Buffer.FirstElement = entry.offset_ / desc.stride_;
		shaderResourceViewDesc.Buffer.NumElements = allocation.count_;
		shaderResourceViewDesc.Buffer.StructureByteStride = desc.stride_;
		d3dDevice->CreateShaderResourceView(buffer.Resource(), &shaderResourceViewDesc, device.ResourceHeap().CPUHandle(allocation.shaderResourceView_));
		return allocation;
	}

	void RenderGraphMemory::Release(RenderDevice& device, RenderGraphAllocation& allocation)
	{
		if (allocation.shaderResourceView_ != SC_INVALID)
		{
			device.ResourceHeap().Release(allocation.resource_, allocation.shaderResourceView_);
		}
		if (allocation.unorderedAccessView_ != SC_INVALID)
		{
			Uint32 mipLevels = allocation.shape_ == RenderResourceShape::Buffer ? 1 : allocation.mipLevels_;
			for (Uint32 mip = 0;mip < mipLevels;++mip)
			{
				device.ResourceHeap().Release(allocation.resource_, allocation.unorderedAccessView_ + mip);
			}
		}
		if (allocation.renderTargetView_ != SC_INVALID)
		{
			device.RenderTargetHeap().Release(allocation.resource_, allocation.renderTargetView_);
		}
		if (allocation.depthStencilView_ != SC_INVALID)
		{
			device.DepthStencilHeap().Release(allocation.resource_, { allocation.depthStencilView_, allocation.depthReadOnlyView_ });
		}
		allocation = RenderGraphAllocation{};
	}

	Bool RenderGraphMemory::Matches(const RenderGraphAllocation& allocation, const RenderGraphResource& resource, const RenderGraphInstance& instance)
	{
		const RenderResourceDesc& desc = resource.desc_;
		Uint32 mipLimit = 1;
		while ((Max(instance.width_, instance.height_) >> mipLimit) > 0)
		{
			++mipLimit;
		}
		return allocation.shape_ == desc.shape_ && allocation.format_ == desc.format_ && allocation.width_ == instance.width_ && allocation.height_ == instance.height_ && allocation.depth_ == instance.depth_ && allocation.mipLevels_ == Min(desc.mipLevels_, mipLimit) && allocation.stride_ == desc.stride_ && allocation.count_ == instance.count_ && allocation.shaderResource_ == resource.shaderResource_ && allocation.unorderedAccess_ == resource.unorderedAccess_ && allocation.renderTarget_ == resource.renderTarget_ && allocation.depthStencil_ == resource.depthStencil_;
	}
}
