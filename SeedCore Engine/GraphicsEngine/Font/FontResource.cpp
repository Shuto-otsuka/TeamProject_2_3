#include <GraphicsEngine/Font/FontResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	FontResource::FontResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void FontResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<Font> FontResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<Font>::null();
		}

		Handle<Font> handle = loaders_.fontLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Font>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	void FontResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Font> handle = assetHandleMap_.at(assetID);
		loaders_.fontLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	void FontResource::Clear()
	{
		for (Handle<Font>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			loaders_.fontLoader_->Clear(handle);
		}
		assetHandleMap_.clear();
		revision_++;
	}

	Font* FontResource::Resolve(const Handle<Font>& handle)
	{
		return loaders_.fontLoader_->Get(handle);
	}
}
