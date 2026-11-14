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
	/**
	* [EN]
	* Creates a texture from the image file at filePath into resource and
	* writes its shader resource view at textureIndex of heap, recording the
	* upload into lane, which the caller has begun and submits. A sibling
	* ".texture" cache newer than the source is read instead of the source; a
	* ".icon", ".logo" or ".texture" file is read as such a cache directly.
	* Nothing is written: the cache is made by TextureResource::Generate.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の画像ファイルから resource にテクスチャを作り、heap の
	* textureIndex にシェーダーリソースビューを書く。アップロードは lane に
	* 積む（Begin と Submit は呼び出し側が行う）。ソースより新しい隣の
	* ".texture" キャッシュがあれば、ソースの代わりにそれを読む。".icon"・
	* ".logo"・".texture" のファイルは、そのままキャッシュとして読む。何も
	* 書かない（キャッシュは TextureResource::Generate が作る）。
	*/
	void TextureCreator::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		std::string extension = std::filesystem::path(filePath.str()).extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		std::filesystem::path sourceFsPath(filePath.str());
		std::filesystem::path cacheFsPath = sourceFsPath;
		cacheFsPath.replace_extension(".texture");

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
			if (std::filesystem::exists(cacheFsPath) && std::filesystem::exists(sourceFsPath) && std::filesystem::last_write_time(cacheFsPath) >= std::filesystem::last_write_time(sourceFsPath))
			{
				BinaryInputArchive archive;
				if (archive.Read(String(cacheFsPath.string())))
				{
					archive.TryField("data", data);
				}
			}
			if (data.empty())
			{
				DynamicArray<Uint8> source = FileUtility::LoadFileBinary(filePath);
				data.assign(source.begin(), source.end());
			}
		}

		if (data.empty())
		{
			SC_LOG_ERROR("テクスチャのロードに失敗しました: {}", filePath.str());
			resource.Reset();
			return;
		}
		CreateTextureMemory(device, lane, heap, data, resource, textureIndex);
	}

	/**
	* [EN]
	* Same as the lane overload, but begins transferLane on copyQueue, submits
	* it and waits for the upload before returning.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* lane 版と同じだが、copyQueue 上で transferLane を Begin し、Submit して
	* アップロードの完了を待ってから戻る。
	*/
	void TextureCreator::CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		transferLane->Begin(device, *copyQueue);
		CreateTexturePath(device, *transferLane, heap, filePath, resource, textureIndex);
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));
	}

	/**
	* [EN]
	* Creates a texture from DDS or WIC image bytes in data into resource and
	* writes its shader resource view at textureIndex of heap, recording the
	* upload of every subresource into lane, which the caller has begun and
	* submits. The bytes are decoded with DirectXTex (DDS first, then WIC);
	* the texture is created in the COMMON state, so the copy queue writes it
	* and the graphics queue reads it without explicit transitions. resource
	* is reset when the bytes cannot be decoded or the texture cannot be
	* created.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* data の DDS か WIC の画像バイト列から resource にテクスチャを作り、heap の
	* textureIndex にシェーダーリソースビューを書く。全サブリソースの
	* アップロードは lane に積む（Begin と Submit は呼び出し側が行う）。
	* デコードは DirectXTex で行う（DDS、次に WIC）。テクスチャは COMMON 状態で
	* 作るので、コピーキューが書き、描画のキューが読むのに明示的な遷移が
	* 要らない。デコードできないか作れなければ resource を空にする。
	*/
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

	/**
	* [EN]
	* Same as the lane overload, but begins transferLane on copyQueue, submits
	* it and waits for the upload before returning.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* lane 版と同じだが、copyQueue 上で transferLane を Begin し、Submit して
	* アップロードの完了を待ってから戻る。
	*/
	void TextureCreator::CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex)
	{
		transferLane->Begin(device, *copyQueue);
		CreateTextureMemory(device, *transferLane, heap, data, resource, textureIndex);
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));
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
