#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates a texture from the image file at filePath into resource and
	* writes its shader resource view at textureIndex of heap, recording
	* the upload into resourceUpload. A sibling ".texture" cache newer
	* than the source is read instead of the source; a ".icon", ".logo"
	* or ".texture" file is read as such a cache directly. Nothing is
	* written: the cache is made by TextureResource::Generate.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の画像ファイルから resource にテクスチャを作り、heap の
	* textureIndex にシェーダーリソースビューを書く。アップロードは
	* resourceUpload に記録する。ソースより新しい隣の ".texture"
	* キャッシュがあれば、ソースの代わりにそれを読む。".icon"・".logo"・
	* ".texture" のファイルは、そのままキャッシュとして読む。何も書かない
	* （キャッシュは TextureResource::Generate が作る）。
	*/
	void TextureCreator::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		HRESULT hr{ S_OK };

		std::string extension = std::filesystem::path(filePath.str()).extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		/// [EN] These extensions are the cache format itself: the encrypted archive's "data" field holds the original image bytes.
		/// [JP] これらの拡張子はキャッシュ形式そのもの。暗号化アーカイブの "data" フィールドが元画像のバイト列を持つ。
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

			/// [EN] The sibling cache is only trusted when it is at least as new as the source, so an edited source is never shadowed by an old cache.
			/// [JP] 隣のキャッシュは、ソース以上に新しいときだけ信用する。編集したソースが古いキャッシュに隠れないようにするため。
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

	/**
	* [EN]
	* Same as the ResourceUploadBatch overload, but submits the upload to
	* cmdQueue and waits for it before returning.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ResourceUploadBatch 版と同じだが、アップロードを cmdQueue へ投入し、
	* 完了を待ってから戻る。
	*/
	void TextureCreator::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		DirectX::ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		CreateTexturePath(device, resourceUpload, heap, filePath, resource, textureIndex);

		/// [EN] The queue is shared with other threads, so it is only held while the batch is submitted.
		/// [JP] キューは他のスレッドと共有しているため、バッチを投入する間だけ確保する。
		std::future<void> uploadFinished;
		{
			auto queueLock = cmdQueue->AcquireLock();
			uploadFinished = resourceUpload.End(cmdQueue->GetCommandQueue());
		}
		uploadFinished.wait();
	}

	/**
	* [EN]
	* Creates a texture from DDS or WIC image bytes in data into resource
	* and writes its shader resource view at textureIndex of heap,
	* recording the upload into resourceUpload. resource is reset when
	* the bytes cannot be decoded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* data の DDS か WIC の画像バイト列から resource にテクスチャを作り、
	* heap の textureIndex にシェーダーリソースビューを書く。アップロードは
	* resourceUpload に記録する。デコードできなければ resource を空にする。
	*/
	void TextureCreator::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
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

	/**
	* [EN]
	* Same as the ResourceUploadBatch overload, but submits the upload to
	* cmdQueue and waits for it before returning.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ResourceUploadBatch 版と同じだが、アップロードを cmdQueue へ投入し、
	* 完了を待ってから戻る。
	*/
	void TextureCreator::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		DirectX::ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		CreateTextureMemory(device, resourceUpload, heap, data, resource, textureIndex);

		/// [EN] The queue is shared with other threads, so it is only held while the batch is submitted.
		/// [JP] キューは他のスレッドと共有しているため、バッチを投入する間だけ確保する。
		std::future<void> uploadFinished;
		{
			auto queueLock = cmdQueue->AcquireLock();
			uploadFinished = resourceUpload.End(cmdQueue->GetCommandQueue());
		}
		uploadFinished.wait();
	}

	/**
	* [EN]
	* Builds a texture from a baked BC7 bitmap: uploads every mip through
	* transferLane on copyQueue, waits for it, and writes a shader
	* resource view into a newly allocated index of heap. The texture is
	* pinned, since it has no file streaming could reload it from. The
	* returned texture has no resource when the bitmap is malformed or the
	* GPU resource could not be created; it is handed to the loader with
	* TextureLoader::Adopt.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 焼き込み済みの BC7 ビットマップからテクスチャを組み立てる。全ミップを
	* copyQueue 上の transferLane でアップロードして完了を待ち、heap に
	* 新しく確保した番号へシェーダーリソースビューを書く。読み直す元の
	* ファイルが無いため、ストリーミングで追い出されないよう固定する。
	* ビットマップが不正か GPU リソースを作れなかった場合、返すテクスチャは
	* リソースを持たない。作ったテクスチャは TextureLoader::Adopt で
	* ローダーへ渡す。
	*/
	Texture TextureCreator::CreateTextureBitmap(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN BindlessHeap* heap, SC_IN const Bitmap& bitmap)
	{
		Texture texture;

		/// [EN] BC7 works in 4x4 blocks, so a dimension that is not a multiple of 4 cannot be a valid baked bitmap.
		/// [JP] BC7 は 4x4 のブロック単位なので、4 の倍数でない大きさは正しく焼き込まれたビットマップではあり得ない。
		if (bitmap.width_ <= 0 || bitmap.height_ <= 0 || bitmap.mipCount_ <= 0 || bitmap.width_ % 4 != 0 || bitmap.height_ % 4 != 0)
		{
			return texture;
		}

		/// [EN] Every mip is 16 bytes per 4x4 block, so the whole chain's size follows from the dimensions alone; data shorter than that is truncated.
		/// [JP] どのミップも 4x4 ブロックあたり 16 バイトなので、チェーン全体の大きさは寸法だけで決まる。それより短いデータは途中で切れている。
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
		textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
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

		/// [EN] Mips are packed back to back in mip order, so each one's offset is the running sum of the sizes before it.
		/// [JP] ミップはミップ順に隙間なく詰まっているため、各ミップの位置はそれより前の大きさの累計になる。
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
		shaderResourceViewDesc.Format = DXGI_FORMAT_BC7_UNORM;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = static_cast<Uint>(bitmap.mipCount_);
		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, heap->CPUHandle(texture.textureIndex_));

		texture.sizeBytes_ = device->GetResourceAllocationInfo(0, 1, &textureDesc).SizeInBytes;
		return texture;
	}
}
