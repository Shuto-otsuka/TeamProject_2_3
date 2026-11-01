#include <GraphicsEngine/Texture/TextureLoader.h>
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
	Handle<Texture> TextureLoader::Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap, String filePath)
	{
		Handle<Texture> handle = pool_.Create();
		Texture* texture = pool_.Get(handle);
		if (!texture)
		{
			return Handle<Texture>::null();
		}

		device_ = device;
		cmdQueue_ = cmdQueue;

		texture->handle_ = handle;
		texture->textureIndex_ = heap->AllocateIndex();
		texture->filePath_ = filePath;

		CreateTexturePath(device, cmdQueue, heap->Heap(), filePath, texture->resource_, texture->textureIndex_);

		if (texture->resource_)
		{
			D3D12_RESOURCE_DESC desc = texture->resource_->GetDesc();
			texture->sizeBytes_ = device->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
			totalResidentBytes_ += texture->sizeBytes_;
		}

		loadedHandles_.push_back(handle);

		return handle;
	}

	Handle<Texture> TextureLoader::Adopt(ID3D12Device* device, D3D12CommandQueue* copyQueue, D3D12TransferLane* transferLane, BindlessHeap* heap, const Bitmap& bitmap)
	{
		if (bitmap.width_ <= 0 || bitmap.height_ <= 0 || bitmap.mipCount_ <= 0 || bitmap.width_ % 4 != 0 || bitmap.height_ % 4 != 0)
		{
			return Handle<Texture>::null();
		}

		Uint64 totalBytes = 0;
		Uint32 mipWidth = static_cast<Uint32>(bitmap.width_);
		Uint32 mipHeight = static_cast<Uint32>(bitmap.height_);
		for (Int mipIndex = 0;mipIndex < bitmap.mipCount_;mipIndex++)
		{
			totalBytes += static_cast<Uint64>(Max((mipWidth + 3) / 4, 1u)) * Max((mipHeight + 3) / 4, 1u) * 16;
			mipWidth = Max(mipWidth / 2, 1u);
			mipHeight = Max(mipHeight / 2, 1u);
		}
		if (bitmap.cacheData_.size() < totalBytes)
		{
			return Handle<Texture>::null();
		}

		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = static_cast<Uint64>(bitmap.width_);
		textureDesc.Height = static_cast<Uint64>(bitmap.height_);
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = static_cast<Uint16>(bitmap.mipCount_);
		textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));
		if (FAILED(hr))
		{
			return Handle<Texture>::null();
		}

		transferLane->Begin(device, *copyQueue);
		Uint64 offset = 0;
		mipWidth = static_cast<Uint32>(bitmap.width_);
		mipHeight = static_cast<Uint32>(bitmap.height_);
		for (Int mipIndex = 0;mipIndex < bitmap.mipCount_;mipIndex++)
		{
			Uint64 mipBytes = static_cast<Uint64>(Max((mipWidth + 3) / 4, 1u)) * Max((mipHeight + 3) / 4, 1u) * 16;
			transferLane->GetStream().Upload(device, transferLane->GetCommandList(), resource.Get(), static_cast<Uint32>(mipIndex), std::as_bytes(std::span(bitmap.cacheData_)).subspan(offset, mipBytes));
			offset += mipBytes;
			mipWidth = Max(mipWidth / 2, 1u);
			mipHeight = Max(mipHeight / 2, 1u);
		}
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));

		Handle<Texture> handle = pool_.Create();
		Texture* texture = pool_.Get(handle);
		if (!texture)
		{
			return Handle<Texture>::null();
		}

		texture->handle_ = handle;
		texture->resource_ = resource;
		texture->pinned_ = true;
		texture->textureIndex_ = heap->AllocateIndex();

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_BC7_UNORM;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = static_cast<Uint>(bitmap.mipCount_);
		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, heap->CPUHandle(texture->textureIndex_));

		texture->sizeBytes_ = device->GetResourceAllocationInfo(0, 1, &textureDesc).SizeInBytes;
		totalResidentBytes_ += texture->sizeBytes_;
		loadedHandles_.push_back(handle);
		return handle;
	}

	Texture* TextureLoader::Get(const Handle<Texture>& handle)
	{
		return pool_.Get(handle);
	}

	void TextureLoader::Clear(Handle<Texture>& handle, BindlessHeap* heap)noexcept
	{
		Texture* texture = pool_.Get(handle);
		if (texture)
		{
			if (texture->resource_)
			{
				heap->Release(std::move(texture->resource_), texture->textureIndex_);
				totalResidentBytes_ -= texture->sizeBytes_;
			}
			auto found = std::ranges::find_if(loadedHandles_, [handle](const auto& candidate){ return candidate == handle; });
			if (found != loadedHandles_.end())
			{
				loadedHandles_.erase(found);
			}
		}
		pool_.Destroy(handle);
	}

	Texture* TextureLoader::Resolve(BindlessHeap* heap, const Handle<Texture>& handle, Uint64 frame)
	{
		Texture* texture = pool_.Get(handle);
		if (!texture)
		{
			return nullptr;
		}

		texture->lastUsedFrame_ = frame;

		if (!texture->resource_ && device_ && cmdQueue_)
		{
			texture->textureIndex_ = heap->AllocateIndex();
			CreateTexturePath(device_, cmdQueue_, heap->Heap(), texture->filePath_, texture->resource_, texture->textureIndex_);
			if (texture->resource_)
			{
				D3D12_RESOURCE_DESC desc = texture->resource_->GetDesc();
				texture->sizeBytes_ = device_->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
				totalResidentBytes_ += texture->sizeBytes_;
			}
		}

		return texture;
	}

	void TextureLoader::EvictBudget(BindlessHeap* heap, Uint64 currentFrame)
	{
		if (totalResidentBytes_ <= budgetBytes_)
		{
			return;
		}

		struct Candidate
		{
			Handle<Texture> handle_;
			Uint64 lastUsedFrame_;
		};
		DynamicArray<Candidate> candidates;
		for (const Handle<Texture>& handle : loadedHandles_)
		{
			Texture* texture = pool_.Get(handle);
			if (texture && texture->resource_ && !texture->pinned_ && texture->lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
			{
				candidates.push_back({ handle, texture->lastUsedFrame_ });
			}
		}

		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
		{
			return a.lastUsedFrame_ < b.lastUsedFrame_;
		});

		for (const Candidate& candidate : candidates)
		{
			if (totalResidentBytes_ <= budgetBytes_)
			{
				break;
			}

			Texture* texture = pool_.Get(candidate.handle_);
			if (!texture)
			{
				continue;
			}

			heap->Release(std::move(texture->resource_), texture->textureIndex_);
			texture->textureIndex_ = 0xFFFFFFFF;
			totalResidentBytes_ -= texture->sizeBytes_;
			texture->sizeBytes_ = 0;
		}
	}

	void TextureLoader::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		HRESULT hr{ S_OK };

		std::string extension = std::filesystem::path(filePath.str()).extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		if (extension == ".icon" || extension == ".logo" || extension == ".texture")
		{
			BinaryInputArchive archive;
			if (archive.Read(filePath))
			{
				DynamicArray<Byte> data;
				archive.TryField("data", data);
				if (!data.empty())
				{
					hr = DirectX::CreateDDSTextureFromMemory(device, resourceUpload, reinterpret_cast<const Uint8*>(data.data()), data.size(), &resource);

					if (FAILED(hr))
					{
						hr = DirectX::CreateWICTextureFromMemory(device, resourceUpload, reinterpret_cast<const Uint8*>(data.data()), data.size(), &resource);
					}
				}
			}
		}
		else
		{
			std::filesystem::path sourceFsPath(filePath.str());
			std::filesystem::path cacheFsPath = sourceFsPath;
			cacheFsPath.replace_extension(".texture");

			Bool loadedFromCache = false;
			if (std::filesystem::exists(cacheFsPath) && std::filesystem::exists(sourceFsPath) && std::filesystem::last_write_time(cacheFsPath) >= std::filesystem::last_write_time(sourceFsPath))
			{
				BinaryInputArchive archive;
				if (archive.Read(String(cacheFsPath.string())))
				{
					DynamicArray<Byte> data;
					archive.TryField("data", data);
					if (!data.empty())
					{
						hr = DirectX::CreateDDSTextureFromMemory(device, resourceUpload, reinterpret_cast<const Uint8*>(data.data()), data.size(), &resource);

						if (FAILED(hr))
						{
							hr = DirectX::CreateWICTextureFromMemory(device, resourceUpload, reinterpret_cast<const Uint8*>(data.data()), data.size(), &resource);
						}

						loadedFromCache = SUCCEEDED(hr) && resource;
					}
				}
			}

			if (!loadedFromCache)
			{
				hr = DirectX::CreateDDSTextureFromFile(device, resourceUpload, filePath.w_str().c_str(), &resource);

				if (FAILED(hr))
				{
					hr = DirectX::CreateWICTextureFromFile(device, resourceUpload, filePath.w_str().c_str(), &resource, false);
				}

				if (SUCCEEDED(hr) && resource)
				{
					DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(filePath);
					if (!sourceBytes.empty())
					{
						BinaryOutputArchive cacheArchive;
						cacheArchive.Field("data", sourceBytes);
						cacheArchive.Write(String(cacheFsPath.string()));
					}
				}
			}
		}

		if (FAILED(hr) || !resource)
		{
			SC_LOG_ERROR("テクスチャのロードに失敗しました: {}", filePath.str());
			return;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = resource->GetDesc().Format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
		Uint64 descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cpuHandle.ptr += textureIndex * descriptorSize;

		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, cpuHandle);
	}

	void TextureLoader::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		DirectX::ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		CreateTexturePath(device, resourceUpload, heap, filePath, resource, textureIndex);

		std::future<void> uploadFinished;
		{
			auto queueLock = cmdQueue->AcquireLock();
			uploadFinished = resourceUpload.End(cmdQueue->GetCommandQueue());
		}
		uploadFinished.wait();
	}

	void TextureLoader::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		HRESULT hr{ S_OK };

		if (!data.empty())
		{
			hr = DirectX::CreateDDSTextureFromMemory(device, resourceUpload, reinterpret_cast<const Uint8*>(data.data()), data.size(), &resource);

			if (FAILED(hr))
			{
				hr = DirectX::CreateWICTextureFromMemory(device, resourceUpload, reinterpret_cast<const Uint8*>(data.data()), data.size(), &resource);
			}
		}

		if (data.empty() || FAILED(hr) || !resource)
		{
			SC_LOG_ERROR("メモリ上の画像からテクスチャを作成できませんでした");
			resource.Reset();
			return;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = resource->GetDesc().Format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
		Uint64 descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cpuHandle.ptr += textureIndex * descriptorSize;

		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, cpuHandle);
	}

	void TextureLoader::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		DirectX::ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		CreateTextureMemory(device, resourceUpload, heap, data, resource, textureIndex);

		std::future<void> uploadFinished;
		{
			auto queueLock = cmdQueue->AcquireLock();
			uploadFinished = resourceUpload.End(cmdQueue->GetCommandQueue());
		}
		uploadFinished.wait();
	}
}
