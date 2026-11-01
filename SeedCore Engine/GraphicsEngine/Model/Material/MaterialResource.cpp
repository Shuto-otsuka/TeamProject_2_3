#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	MaterialResource::MaterialResource(GraphicsLoaderSystem& loaders, TextureResource& textureResource) : loaders_(loaders), textureResource_(textureResource)
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

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<BakedSurface>::null();
		}

		Handle<BakedSurface> handle = loaders_.materialLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<BakedSurface>::null();
		}

		if (BakedSurface* bakedSurface = loaders_.materialLoader_->Get(handle))
		{
			Size textureCount = Min(bakedSurface->textureAssetIDs_.size(), bakedSurface->textures_.size());
			for (Size textureIndex = 0; textureIndex < textureCount; textureIndex++)
			{
				textureResource_.Adopt(bakedSurface->textureAssetIDs_[textureIndex], bakedSurface->textures_[textureIndex], thread);
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

	Handle<BakedSurface> MaterialResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<BakedSurface>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	Surface* MaterialResource::Resolve(const Handle<BakedSurface>& handle)
	{
		BakedSurface* bakedSurface = loaders_.materialLoader_->Get(handle);
		return bakedSurface ? &bakedSurface->surface_ : nullptr;
	}

	Bool MaterialResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}

	Uint32 MaterialResource::Revision()const
	{
		return revision_;
	}
}
