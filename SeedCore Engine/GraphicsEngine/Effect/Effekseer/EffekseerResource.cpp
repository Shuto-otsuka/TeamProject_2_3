#include <GraphicsEngine/Effect/Effekseer/EffekseerResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	EffekseerResource::EffekseerResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void EffekseerResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}

	Handle<EffekseerEffect> EffekseerResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<EffekseerEffect>::null();
		}

		Handle<EffekseerEffect> handle = loaders_.effekseerLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<EffekseerEffect>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	void EffekseerResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<EffekseerEffect> handle = assetHandleMap_.at(assetID);
		loaders_.effekseerLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
	}

	Handle<EffekseerEffect> EffekseerResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<EffekseerEffect>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	EffekseerEffect* EffekseerResource::Resolve(const Handle<EffekseerEffect>& handle)
	{
		return loaders_.effekseerLoader_->Get(handle);
	}

	Bool EffekseerResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}
}
