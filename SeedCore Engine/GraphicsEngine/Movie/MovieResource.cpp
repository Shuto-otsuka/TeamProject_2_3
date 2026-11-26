#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/Movie/Video.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	MovieResource::MovieResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void MovieResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<Video> MovieResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<Video>::null();
		}

		Handle<Video> handle = loaders_.movieLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Video>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	void MovieResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Video> handle = assetHandleMap_.at(assetID);
		loaders_.movieLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	void MovieResource::Clear()
	{
		for (Handle<Video>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			loaders_.movieLoader_->Clear(handle);
		}
		assetHandleMap_.clear();
		revision_++;
	}

	Bool MovieResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".movie")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".movie");
		if (!loaders_.movieLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

	Video* MovieResource::Resolve(const Handle<Video>& handle)
	{
		return loaders_.movieLoader_->Get(handle);
	}
}
