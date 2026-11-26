#include <External/DirectXTex/Include/DirectXTex.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/File/FileUtility.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	void TextureCreator::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		std::string extension = std::filesystem::path(filePath.str()).extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		DynamicArray<Byte> data;
		if (extension == ".icon" || extension == ".logo" || extension == ".texture")
		{
			BinaryInputArchive archive;
			if (archive.Read(filePath))
			{
				archive.TryField("data", data);
			}
		}
		else
		{
			DynamicArray<Uint8> source = FileUtility::LoadFileBinary(filePath);
			data.assign(source.begin(), source.end());
		}

		if (data.empty())
		{
			SC_LOG_ERROR("テクスチャのロードに失敗しました: {}", filePath.str());
			resource.Reset();
			return;
		}
		CreateTextureMemory(device, lane, heap, data, resource, textureIndex);
	}

	void TextureCreator::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		transferLane->Begin(device, *copyQueue);
		CreateTexturePath(device, *transferLane, heap, filePath, resource, textureIndex);
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));
	}

	void TextureCreator::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		resource.Reset();

		DirectX::ScratchImage image;
		HRESULT hr = E_FAIL;
		if (!data.empty())
		{
			hr = DirectX::LoadFromDDSMemory(reinterpret_cast<const Uint8*>(data.data()), data.size(), DirectX::DDS_FLAGS_NONE, nullptr, image);
			if (FAILED(hr))
			{
				hr = DirectX::LoadFromWICMemory(reinterpret_cast<const Uint8*>(data.data()), data.size(), DirectX::WIC_FLAGS_NONE, nullptr, image);
			}
		}
		if (FAILED(hr))
		{
			SC_LOG_ERROR("メモリ上の画像からテクスチャを作成できませんでした");
			return;
		}

		const DirectX::TexMetadata& metadata = image.GetMetadata();
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = static_cast<Uint64>(metadata.width);
		resourceDesc.Height = static_cast<Uint>(metadata.height);
		resourceDesc.DepthOrArraySize = static_cast<Uint16>(metadata.arraySize);
		resourceDesc.MipLevels = static_cast<Uint16>(metadata.mipLevels);
		resourceDesc.Format = metadata.format;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
		if (FAILED(hr) || !resource)
		{
			SC_LOG_ERROR("メモリ上の画像からテクスチャを作成できませんでした");
			resource.Reset();
			return;
		}

		for (Size item = 0;item < metadata.arraySize;++item)
		{
			for (Size mip = 0;mip < metadata.mipLevels;++mip)
			{
				const DirectX::Image* subimage = image.GetImage(mip, item, 0);
				Uint32 subresource = static_cast<Uint32>(mip + item * metadata.mipLevels);
				lane.GetStream().Upload(device, lane.GetCommandList(), resource.Get(), subresource, std::as_bytes(std::span(subimage->pixels, subimage->slicePitch)));
			}
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = metadata.format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = static_cast<Uint>(metadata.mipLevels);
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
		Uint64 descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cpuHandle.ptr += textureIndex * descriptorSize;

		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, cpuHandle);
	}

	void TextureCreator::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		transferLane->Begin(device, *copyQueue);
		CreateTextureMemory(device, *transferLane, heap, data, resource, textureIndex);
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));
	}

	Texture TextureCreator::CreateTextureBitmap(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN BindlessHeap* heap, SC_IN const Bitmap& bitmap, SC_IN DXGI_FORMAT format)
	{
		Texture texture;

		if (bitmap.width_ <= 0 || bitmap.height_ <= 0 || bitmap.mipCount_ <= 0 || bitmap.width_ % 4 != 0 || bitmap.height_ % 4 != 0)
		{
			return texture;
		}

		Uint64 totalBytes = 0;
		Uint32 mipWidth = static_cast<Uint32>(bitmap.width_);
		Uint32 mipHeight = static_cast<Uint32>(bitmap.height_);
		for (Int mipIndex = 0; mipIndex < bitmap.mipCount_; mipIndex++)
		{
			totalBytes += static_cast<Uint64>(Max((mipWidth + 3) / 4, 1u)) * Max((mipHeight + 3) / 4, 1u) * 16;
			mipWidth = Max(mipWidth / 2, 1u);
			mipHeight = Max(mipHeight / 2, 1u);
		}
		if (bitmap.cacheData_.size() < totalBytes)
		{
			return texture;
		}

		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = static_cast<Uint64>(bitmap.width_);
		textureDesc.Height = static_cast<Uint64>(bitmap.height_);
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = static_cast<Uint16>(bitmap.mipCount_);
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));
		if (FAILED(hr))
		{
			return texture;
		}

		transferLane->Begin(device, *copyQueue);
		Uint64 offset = 0;
		mipWidth = static_cast<Uint32>(bitmap.width_);
		mipHeight = static_cast<Uint32>(bitmap.height_);
		for (Int mipIndex = 0; mipIndex < bitmap.mipCount_; mipIndex++)
		{
			Uint64 mipBytes = static_cast<Uint64>(Max((mipWidth + 3) / 4, 1u)) * Max((mipHeight + 3) / 4, 1u) * 16;
			transferLane->GetStream().Upload(device, transferLane->GetCommandList(), resource.Get(), static_cast<Uint32>(mipIndex), std::as_bytes(std::span(bitmap.cacheData_)).subspan(offset, mipBytes));
			offset += mipBytes;
			mipWidth = Max(mipWidth / 2, 1u);
			mipHeight = Max(mipHeight / 2, 1u);
		}
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));

		texture.resource_ = resource;
		texture.pinned_ = true;
		texture.textureIndex_ = heap->AllocateIndex();

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = static_cast<Uint>(bitmap.mipCount_);
		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, heap->CPUHandle(texture.textureIndex_));

		texture.sizeBytes_ = device->GetResourceAllocationInfo(0, 1, &textureDesc).SizeInBytes;
		return texture;
	}
}
