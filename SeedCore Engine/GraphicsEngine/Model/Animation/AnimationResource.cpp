#include <GraphicsEngine/Model/Animation/AnimationResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	AnimationResource::AnimationResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void AnimationResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}

	Handle<Animation> AnimationResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Animation>::null();
		}

		Handle<Animation> handle = loaders_.animationLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<Animation>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	void AnimationResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Animation> handle = assetHandleMap_.at(assetID);
		loaders_.animationLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
	}

	Handle<Animation> AnimationResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Animation>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	Animation* AnimationResource::Resolve(const Handle<Animation>& handle)
	{
		return loaders_.animationLoader_->Get(handle);
	}

	Bool AnimationResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}
}
