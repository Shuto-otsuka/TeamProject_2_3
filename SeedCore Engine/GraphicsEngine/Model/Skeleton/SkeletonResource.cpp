#include <GraphicsEngine/Model/Skeleton/SkeletonResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	SkeletonResource::SkeletonResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void SkeletonResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}

	Handle<SkeletonRig> SkeletonResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<SkeletonRig>::null();
		}

		Handle<SkeletonRig> handle = loaders_.skeletonLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<SkeletonRig>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	void SkeletonResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<SkeletonRig> handle = assetHandleMap_.at(assetID);
		loaders_.skeletonLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
	}

	Handle<SkeletonRig> SkeletonResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<SkeletonRig>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	SkeletonRig* SkeletonResource::Resolve(const Handle<SkeletonRig>& handle)
	{
		return loaders_.skeletonLoader_->Get(handle);
	}

	Bool SkeletonResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}
}
