#include <GraphicsEngine/Effect/EffectResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	EffectResource::EffectResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void EffectResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<EffectAsset> EffectResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<EffectAsset>::null();
		}

		Handle<EffectAsset> handle = loaders_.effectLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<EffectAsset>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	void EffectResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<EffectAsset> handle = assetHandleMap_.at(assetID);
		loaders_.effectLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	Bool EffectResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		EffectAsset* effect = Resolve(GetHandle(assetID));
		if (!effect)
		{
			return false;
		}
		return loaders_.effectLoader_->Bake(*effect, filePath);
	}

	EffectAsset* EffectResource::Resolve(const Handle<EffectAsset>& handle)
	{
		return loaders_.effectLoader_->Get(handle);
	}
}