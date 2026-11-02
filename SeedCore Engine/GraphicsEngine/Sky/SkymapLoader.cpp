#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <External/DirectXTex/Include/DirectXTex.h>
#include <GraphicsEngine/Sky/SkymapLoader.h>
#include <GraphicsEngine/Sky/SkymapCache.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the device context skymaps are uploaded with and the
	* bindless heap their textures are registered in.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スカイマップをアップロードするデバイスのコンテキストと、テクスチャの
	* 登録先のバインドレスヒープを受け取る。
	*/
	SkymapLoader::SkymapLoader(D3D12Context& context, BindlessHeap& heap) :context_(context), heap_(heap)
	{
		/// No Code
	}

	/**
	* [EN]
	* Reads the ".hdr" source or ".skymap" cache at filePath, uploads it
	* through the direct queue and returns the handle of the new Skymap,
	* or a null handle when it cannot be read or uploaded. Nothing is
	* written. thread is unused: the upload waits on the direct queue.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の ".hdr" ソースか ".skymap" キャッシュを読み、Direct キュー
	* でアップロードして、新しい Skymap のハンドルを返す。読めない、または
	* アップロードできない場合は null のハンドルを返す。何も書き出さない。
	* thread は使わない（アップロードは Direct キューで待つため）。
	*/
	Handle<Skymap> SkymapLoader::Load(String filePath, LoadThread thread)
	{
		/// [EN] The GPU objects the upload below works with, taken from the dependencies this loader was built with.
		/// [JP] 以下のアップロードが使う GPU のオブジェクト。このローダーが構築時に受け取った依存から取る。
		ID3D12Device* device = context_.GetDevice();
		D3D12CommandQueue* cmdQueue = context_.GetDirectQueue();
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
		DynamicArray<Uint8> cachePixels;
		Bool fromCache = (extension == ".skymap");

		if (fromCache)
		{
			SkymapCacheHeader header{};
			if (!ReadSkymapCache(filePath, header, cachePixels))
			{
				SC_LOG_ERROR("スカイマップキャッシュのロードに失敗しました: {}", filePath.str());
				pool_.Destroy(handle);
				return Handle<Skymap>::null();
			}

			format = static_cast<DXGI_FORMAT>(header.format_);
			width = header.width_;
			height = header.height_;
			rowPitch = header.rowPitch_;
			slicePitch = header.dataSize_;
			pixels = cachePixels.data();
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

		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&skymap->resource_));
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

		D3D12_SUBRESOURCE_DATA subresource{};
		subresource.pData = pixels;
		subresource.RowPitch = static_cast<LONG_PTR>(rowPitch);
		subresource.SlicePitch = static_cast<LONG_PTR>(rowPitch) * static_cast<LONG_PTR>(height);

		DirectX::ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();
		resourceUpload.Upload(skymap->resource_.Get(), 0, &subresource, 1);
		resourceUpload.Transition(skymap->resource_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		std::future<void> uploadFinished;
		{
			auto queueLock = cmdQueue->AcquireLock();
			uploadFinished = resourceUpload.End(cmdQueue->GetCommandQueue());
		}
		uploadFinished.wait();

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

	/**
	* [EN]
	* Hands the Skymap's texture to the heap's deferred release, then
	* frees its slot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Skymap のテクスチャをヒープの遅延解放へ渡してから、スロットを
	* 解放する。
	*/
	void SkymapLoader::Clear(Handle<Skymap>& handle)noexcept
	{
		Skymap* skymap = pool_.Get(handle);
		if (skymap)
		{
			/// [EN] pool_.Destroy below runs the Skymap's destructor right here,
			///      so the equirect texture must be handed to the deferred ring
			///      first - the frames still in flight are sampling it.
			/// [JP] 下の pool_.Destroy はこの場で Skymap のデストラクタを走らせる
			///      ため、先に equirect テクスチャを遅延回収リングへ渡す必要が
			///      ある — インフライトのフレームがまだサンプリングしている。
			heap_.Release(std::move(skymap->resource_), skymap->shaderResourceViewIndex_);
			skymap->shaderResourceViewIndex_ = SC_INVALID;
		}

		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Decodes the ".hdr" source at sourcePath and writes its pixels to the
	* ".skymap" cache at cachePath, so the skymap can be shipped and
	* loaded without the source. Returns false when the source cannot be
	* decoded or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* sourcePath の ".hdr" ソースをデコードし、そのピクセルを cachePath の
	* ".skymap" キャッシュへ書く。ソース無しでスカイマップを配布・読み込み
	* できるようにするため。ソースをデコードできない、または書き込みに
	* 失敗した場合は false を返す。
	*/
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

		/// [EN] The header records exactly what Load needs to recreate the texture without decoding the HDR again.
		/// [JP] ヘッダーには、HDR をデコードし直さずに Load がテクスチャを作り直すのに要るものをそのまま記録する。
		SkymapCacheHeader header{};
		std::memcpy(header.magic_, skymapCacheMagic_, sizeof(header.magic_));
		header.version_ = skymapCacheVersion_;
		header.format_ = static_cast<Uint32>(metadata.format);
		header.width_ = static_cast<Uint32>(metadata.width);
		header.height_ = static_cast<Uint32>(metadata.height);
		header.rowPitch_ = static_cast<Uint32>(sourceImage->rowPitch);
		header.dataSize_ = static_cast<Uint32>(sourceImage->slicePitch);
		return WriteSkymapCache(cachePath, header, sourceImage->pixels);
	}
}
