#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/Material/MaterialBaker.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	MaterialResource::MaterialResource(GraphicsLoaderSystem& loaders, TextureResource& textureResource, ModelResource& modelResource, D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor) :loaders_(loaders), textureResource_(textureResource), modelResource_(modelResource), context_(context), heap_(heap), processor_(processor)
	{
		/// No Code
	}

	void MaterialResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<BakedSurface> MaterialResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<BakedSurface>::null();
		}

		Handle<BakedSurface> handle = loaders_.materialLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<BakedSurface>::null();
		}

		if (BakedSurface* bakedSurface = loaders_.materialLoader_->Get(handle))
		{
			Size textureCount = Min(bakedSurface->textureAssetIDs_.size(), bakedSurface->textures_.size());
			for (Size textureIndex = 0; textureIndex < textureCount; textureIndex++)
			{
				Uint32 textureAssetID = bakedSurface->textureAssetIDs_[textureIndex];

				if (textureResource_.Contains(textureAssetID))
				{
					Texture* existing = textureResource_.Resolve(textureResource_.GetHandle(textureAssetID));
					if (existing && !existing->FilePath().view().empty())
					{
						continue;
					}
					textureResource_.Unload(textureAssetID);
				}

				Texture texture = TextureCreator::CreateTextureBitmap(context_.GetDevice(), context_.GetCopyQueue(), context_.GetLoadTransferLane(thread), &heap_, bakedSurface->textures_[textureIndex]);
				if (!texture.Resource())
				{
					continue;
				}
				textureResource_.Adopt(textureAssetID, loaders_.textureLoader_->Adopt(std::move(texture)));
			}

			bakedSurface->textures_.clear();
			bakedSurface->textures_.shrink_to_fit();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	void MaterialResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<BakedSurface> handle = assetHandleMap_.at(assetID);
		loaders_.materialLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	Bool MaterialResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		Crister* crister = modelResource_.Resolve(modelResource_.Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister)
		{
			return false;
		}

		std::span<const Surface> materials = crister->Surfaces();
		if (materials.empty())
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path directory = modelPath.parent_path() / (modelPath.stem().string() + ".Materials");
		std::error_code error;
		std::filesystem::create_directories(directory, error);

		std::set<std::string> usedNames;
		Bool allWritten = true;
		for (Size materialIndex = 0; materialIndex < materials.size(); materialIndex++)
		{
			const Surface& material = materials[materialIndex];

			std::string stem = material.name_.empty() ? ("Material_" + std::to_string(materialIndex)) : material.name_;
			for (char& character : stem)
			{
				if (std::string_view("\\/:*?\"<>|").find(character) != std::string_view::npos || static_cast<Uchar>(character) < 0x20)
				{
					character = '_';
				}
			}
			if (usedNames.contains(stem))
			{
				stem += "_" + std::to_string(materialIndex);
			}
			usedNames.insert(stem);

			std::filesystem::path filePath = directory / (stem + ".material");
			BakedSurface bakedSurface;
			bakedSurface.surface_ = material;
			if (!loaders_.materialLoader_->Bake(bakedSurface, String(filePath.string())))
			{
				allWritten = false;
				continue;
			}
			writtenPaths.push_back(String(filePath.string()));
		}

		return allWritten;
	}

	Bool MaterialResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		BakedSurface* loaded = Resolve(GetHandle(assetID));
		if (!loaded)
		{
			return false;
		}

		BakedSurface bakedSurface;
		if (AssetRecord* asset = context.cache_.Record(assetID))
		{
			BinaryInputArchive archive;
			if (archive.Read(asset->fullpath_))
			{
				bakedSurface.Serialize(archive);
			}
		}
		bakedSurface.surface_ = loaded->surface_;

		Size addedCount = Min(loaded->textureAssetIDs_.size(), loaded->textures_.size());
		for (Size addedIndex = 0; addedIndex < addedCount; addedIndex++)
		{
			auto found = std::ranges::find(bakedSurface.textureAssetIDs_, loaded->textureAssetIDs_[addedIndex]);
			if (found != bakedSurface.textureAssetIDs_.end() && static_cast<Size>(std::distance(bakedSurface.textureAssetIDs_.begin(), found)) < bakedSurface.textures_.size())
			{
				bakedSurface.textures_[std::distance(bakedSurface.textureAssetIDs_.begin(), found)] = loaded->textures_[addedIndex];
			}
			else
			{
				bakedSurface.textureAssetIDs_.push_back(loaded->textureAssetIDs_[addedIndex]);
				bakedSurface.textures_.push_back(loaded->textures_[addedIndex]);
			}
		}

		MaterialBaker baker;
		baker.BakeReferenced(context_.GetDevice(), context_.GetDirectQueue(), processor_, context.cache_, bakedSurface);
		return loaders_.materialLoader_->Bake(bakedSurface, filePath);
	}

	BakedSurface* MaterialResource::Resolve(const Handle<BakedSurface>& handle)
	{
		return loaders_.materialLoader_->Get(handle);
	}
}
