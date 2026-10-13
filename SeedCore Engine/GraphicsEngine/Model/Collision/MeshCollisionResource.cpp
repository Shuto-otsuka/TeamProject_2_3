#include <GraphicsEngine/Model/Collision/MeshCollisionResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	MeshCollisionResource::MeshCollisionResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void MeshCollisionResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}

	Handle<MeshCollision> MeshCollisionResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<MeshCollision>::null();
		}

		Handle<MeshCollision> handle = loaders_.meshCollisionLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<MeshCollision>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	void MeshCollisionResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<MeshCollision> handle = assetHandleMap_.at(assetID);
		loaders_.meshCollisionLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
	}

	Handle<MeshCollision> MeshCollisionResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<MeshCollision>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	MeshCollision* MeshCollisionResource::Resolve(const Handle<MeshCollision>& handle)
	{
		return loaders_.meshCollisionLoader_->Get(handle);
	}

	Bool MeshCollisionResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}
}
