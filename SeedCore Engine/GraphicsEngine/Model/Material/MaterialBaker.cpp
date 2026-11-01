#include <GraphicsEngine/Model/Material/MaterialBaker.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerCompressor.h>
#include <External/DirectXTex/Include/DirectXTex.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/File/FileUtility.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	void MaterialBaker::BakeReferenced(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, BakedSurface& bakedSurface)const
	{
		const Surface& surface = bakedSurface.surface_;
		Uint32 referenceIDs[] = { surface.baseColorTextureAssetID_, surface.normalTextureAssetID_, surface.metallicRoughnessTextureAssetID_, surface.occlusionTextureAssetID_, surface.emissiveTextureAssetID_ };

		for (Size textureIndex = bakedSurface.textureAssetIDs_.size(); textureIndex-- > 0;)
		{
			if (std::ranges::find(referenceIDs, bakedSurface.textureAssetIDs_[textureIndex]) == std::end(referenceIDs))
			{
				bakedSurface.textureAssetIDs_.erase(bakedSurface.textureAssetIDs_.begin() + textureIndex);
				bakedSurface.textures_.erase(bakedSurface.textures_.begin() + textureIndex);
			}
		}

		DynamicArray<Uint32> freshIDs;
		DynamicArray<Bitmap> bitmaps;
		for (Uint32 assetID : referenceIDs)
		{
			AssetRecord* asset = assetID != 0 ? cache.GetAsset(assetID) : nullptr;
			if (!asset || std::ranges::contains(freshIDs, assetID))
			{
				continue;
			}

			std::filesystem::path path(asset->fullpath_.c_str());
			std::string extension = path.extension().string();
			std::ranges::transform(extension, extension.begin(), [](Uchar character) { return static_cast<Char>(std::tolower(character)); });

			DynamicArray<Byte> fileBytes;
			if (extension == ".texture")
			{
				BinaryInputArchive archive;
				if (archive.Read(asset->fullpath_))
				{
					archive.TryField("data", fileBytes);
				}
			}
			else
			{
				DynamicArray<Uint8> loadedBytes = FileUtility::LoadFileBinary(asset->fullpath_);
				fileBytes.assign(loadedBytes.begin(), loadedBytes.end());
			}

			if (fileBytes.empty())
			{
				SC_LOG_WARNING("マテリアルの Bake: テクスチャ {} を読み込めませんでした。", asset->fullpath_.c_str());
				continue;
			}

			const Uint8* bytes = reinterpret_cast<const Uint8*>(fileBytes.data());
			DirectX::TexMetadata metadata{};
			DirectX::ScratchImage loadedImage;
			HRESULT hr = DirectX::LoadFromDDSMemory(bytes, fileBytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, loadedImage);
			if (FAILED(hr))
			{
				hr = DirectX::LoadFromTGAMemory(bytes, fileBytes.size(), &metadata, loadedImage);
			}
			if (FAILED(hr))
			{
				hr = DirectX::LoadFromWICMemory(bytes, fileBytes.size(), DirectX::WIC_FLAGS_NONE, &metadata, loadedImage);
			}
			if (FAILED(hr))
			{
				SC_LOG_WARNING("マテリアルの Bake: テクスチャ {} をデコードできませんでした(HRESULT=0x{:08X})。", asset->fullpath_.c_str(), static_cast<Uint32>(hr));
				continue;
			}

			DirectX::ScratchImage rgbaImage;
			const DirectX::Image* image = loadedImage.GetImage(0, 0, 0);
			if (metadata.format != DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				if (DirectX::IsCompressed(metadata.format))
				{
					hr = DirectX::Decompress(*image, DXGI_FORMAT_R8G8B8A8_UNORM, rgbaImage);
				}
				else
				{
					hr = DirectX::Convert(*image, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgbaImage);
				}
				if (FAILED(hr))
				{
					SC_LOG_WARNING("マテリアルの Bake: テクスチャ {} を RGBA8 に変換できませんでした(HRESULT=0x{:08X})。", asset->fullpath_.c_str(), static_cast<Uint32>(hr));
					continue;
				}
				image = rgbaImage.GetImage(0, 0, 0);
			}

			Bitmap& bitmap = bitmaps.emplace_back();
			bitmap.name_ = path.stem().string();
			bitmap.width_ = static_cast<Int>(image->width);
			bitmap.height_ = static_cast<Int>(image->height);
			bitmap.component_ = 4;
			bitmap.bits_ = 8;
			bitmap.cacheData_.resize(image->width * image->height * 4);
			for (Size row = 0; row < image->height; row++)
			{
				memcpy(bitmap.cacheData_.data() + row * image->width * 4, image->pixels + row * image->rowPitch, image->width * 4);
			}
			freshIDs.push_back(assetID);
		}

		if (bitmaps.empty())
		{
			return;
		}

		CristerCompressor compressor;
		compressor.BakeBitmap(device, cmdQueue, bc7Shader, bitmaps);

		for (Size freshIndex = 0; freshIndex < freshIDs.size(); freshIndex++)
		{
			if (bitmaps[freshIndex].cacheData_.empty() || bitmaps[freshIndex].mipCount_ <= 0)
			{
				continue;
			}

			auto found = std::ranges::find(bakedSurface.textureAssetIDs_, freshIDs[freshIndex]);
			if (found != bakedSurface.textureAssetIDs_.end())
			{
				bakedSurface.textures_[std::distance(bakedSurface.textureAssetIDs_.begin(), found)] = std::move(bitmaps[freshIndex]);
			}
			else
			{
				bakedSurface.textureAssetIDs_.push_back(freshIDs[freshIndex]);
				bakedSurface.textures_.push_back(std::move(bitmaps[freshIndex]));
			}
		}
	}

	void MaterialBaker::BakeEmbedded(const Crister& crister, Uint32 modelAssetID, BakedSurface& bakedSurface)const
	{
		std::span<const Bitmap> bitmaps = crister.Bitmaps();
		Surface& surface = bakedSurface.surface_;
		Uint32* assetIDs[] = { &surface.baseColorTextureAssetID_, &surface.normalTextureAssetID_, &surface.metallicRoughnessTextureAssetID_, &surface.occlusionTextureAssetID_, &surface.emissiveTextureAssetID_ };
		Uint32* textureIndices[] = { &surface.baseColorTextureIndex_, &surface.normalTextureIndex_, &surface.metallicRoughnessTextureIndex_, &surface.occlusionTextureIndex_, &surface.emissiveTextureIndex_ };

		for (Size slotIndex = 0; slotIndex < std::size(assetIDs); slotIndex++)
		{
			Uint32 textureIndex = *textureIndices[slotIndex];
			if (*assetIDs[slotIndex] != 0 || textureIndex >= bitmaps.size() || bitmaps[textureIndex].cacheData_.empty())
			{
				continue;
			}

			Uint32 assetID = 2166136261u;
			assetID = (assetID ^ modelAssetID) * 16777619u;
			assetID = (assetID ^ textureIndex) * 16777619u;

			if (!std::ranges::contains(bakedSurface.textureAssetIDs_, assetID))
			{
				bakedSurface.textureAssetIDs_.push_back(assetID);
				bakedSurface.textures_.push_back(bitmaps[textureIndex]);
			}

			*assetIDs[slotIndex] = assetID;
			*textureIndices[slotIndex] = SC_INVALID;
		}
	}
}