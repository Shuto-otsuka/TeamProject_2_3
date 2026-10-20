#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	MaterialResource::MaterialResource(GraphicsLoaderSystem& loaders) : loaders_(loaders)
	{
		/// No Code
	}

	void MaterialResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}

	Handle<Surface> MaterialResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Surface>::null();
		}

		Handle<Surface> handle = loaders_.materialLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<Surface>::null();
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

		Handle<Surface> handle = assetHandleMap_.at(assetID);
		loaders_.materialLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	Handle<Surface> MaterialResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Surface>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	Surface* MaterialResource::Resolve(const Handle<Surface>& handle)
	{
		return loaders_.materialLoader_->Get(handle);
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
