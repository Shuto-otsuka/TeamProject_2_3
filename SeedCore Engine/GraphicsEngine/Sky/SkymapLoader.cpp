#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <External/DirectXTex/Include/DirectXTex.h>
#include <GraphicsEngine/Sky/SkymapLoader.h>
#include <GraphicsEngine/Sky/SkymapAsset.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	SkymapLoader::SkymapLoader(D3D12Context& context, BindlessHeap& heap) :context_(context), heap_(heap)
	{
		/// No Code
	}

	Handle<Skymap> SkymapLoader::Load(String filePath, LoadThread thread)
	{
		ID3D12Device* device = context_.GetDevice();
		D3D12CommandQueue* copyQueue = context_.GetCopyQueue();
		D3D12TransferLane* transferLane = context_.GetLoadTransferLane(thread);
		BindlessHeap* heap = &heap_;

		Handle<Skymap> handle = pool_.Create();
		Skymap* skymap = pool_.Get(handle);
		if (!skymap)
		{
			return Handle<Skymap>::null();
		}

		skymap->handle_ = handle;

		std::string extension = std::filesystem::path(filePath.str()).extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		Uint width = 0;
		Uint height = 0;
		Uint rowPitch = 0;
		Uint slicePitch = 0;
		const void* pixels = nullptr;

		DirectX::ScratchImage image;
		SkymapAsset asset;
		Bool fromCache = (extension == ".skymap");

		if (fromCache)
		{
			BinaryInputArchive archive;
			if (!archive.Read(filePath))
			{
				SC_LOG_ERROR("スカイマップキャッシュのロードに失敗しました: {}", filePath.str());
				pool_.Destroy(handle);
				return Handle<Skymap>::null();
			}
			asset.Serialize(archive);
			if (asset.pixels_.empty())
			{
				SC_LOG_ERROR("スカイマップキャッシュのロードに失敗しました: {}", filePath.str());
				pool_.Destroy(handle);
				return Handle<Skymap>::null();
			}

			format = static_cast<DXGI_FORMAT>(asset.format_);
			width = asset.width_;
			height = asset.height_;
			rowPitch = asset.rowPitch_;
			slicePitch = static_cast<Uint>(asset.pixels_.size());
			pixels = asset.pixels_.data();
		}
		else
		{
			HRESULT loadResult = DirectX::LoadFromHDRFile(filePath.w_str().c_str(), nullptr, image);
			if (FAILED(loadResult))
			{
				SC_LOG_ERROR("スカイマップ HDR のロードに失敗しました: {}", filePath.str());
				pool_.Destroy(handle);
				return Handle<Skymap>::null();
			}

			const DirectX::TexMetadata& metadata = image.GetMetadata();
			const DirectX::Image* sourceImage = image.GetImage(0, 0, 0);
			if (!sourceImage)
			{
				SC_LOG_ERROR("スカイマップ equirect の画像取得に失敗しました: {}", filePath.str());
				pool_.Destroy(handle);
				return Handle<Skymap>::null();
			}

			format = metadata.format;
			width = static_cast<Uint>(metadata.width);
			height = static_cast<Uint>(metadata.height);
			rowPitch = static_cast<Uint>(sourceImage->rowPitch);
			slicePitch = static_cast<Uint>(sourceImage->slicePitch);
			pixels = sourceImage->pixels;
		}

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = static_cast<Uint64>(width);
		resourceDesc.Height = height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = format;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&skymap->resource_));
		if (FAILED(hr) || !skymap->resource_)
		{
			SC_LOG_ERROR("スカイマップ equirect テクスチャの生成に失敗しました: {}", filePath.str());
			pool_.Destroy(handle);
			return Handle<Skymap>::null();
		}
#if SC_DEBUG
		skymap->resource_->SetName(L"Skymap_Equirect");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(skymap->resource_.Get());
#endif

		transferLane->Begin(device, *copyQueue);
		transferLane->GetStream().Upload(device, transferLane->GetCommandList(), skymap->resource_.Get(), 0, std::span<const std::byte>(static_cast<const std::byte*>(pixels),static_cast<Size>(rowPitch) * height));
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));

		skymap->shaderResourceViewIndex_ = heap->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(skymap->resource_.Get(), &shaderResourceViewDesc, heap->CPUHandle(skymap->shaderResourceViewIndex_));

		return handle;
	}

	void SkymapLoader::Clear(Handle<Skymap>& handle)noexcept
	{
		Skymap* skymap = pool_.Get(handle);
		if (skymap)
		{
			heap_.Release(std::move(skymap->resource_), skymap->shaderResourceViewIndex_);
			skymap->shaderResourceViewIndex_ = SC_INVALID;
		}

		pool_.Destroy(handle);
	}

	Bool SkymapLoader::Bake(String sourcePath, String cachePath)
	{
		DirectX::ScratchImage image;
		if (FAILED(DirectX::LoadFromHDRFile(sourcePath.w_str().c_str(), nullptr, image)))
		{
			return false;
		}

		const DirectX::TexMetadata& metadata = image.GetMetadata();
		const DirectX::Image* sourceImage = image.GetImage(0, 0, 0);
		if (!sourceImage)
		{
			return false;
		}

		SkymapAsset asset;
		asset.format_ = static_cast<Uint32>(metadata.format);
		asset.width_ = static_cast<Uint32>(metadata.width);
		asset.height_ = static_cast<Uint32>(metadata.height);
		asset.rowPitch_ = static_cast<Uint32>(sourceImage->rowPitch);
		asset.pixels_.assign(sourceImage->pixels, sourceImage->pixels + sourceImage->slicePitch);

		BinaryOutputArchive archive;
		asset.Serialize(archive);
		if (!archive.Write(cachePath))
		{
			SC_LOG_ERROR("スカイマップキャッシュの書き出しに失敗しました: {}", cachePath.str());
			return false;
		}
		return true;
	}
}
