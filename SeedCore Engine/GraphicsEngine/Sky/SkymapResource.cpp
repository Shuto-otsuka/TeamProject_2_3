#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Sky/SkymapLoader.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	SkymapResource::SkymapResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void SkymapResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<Skymap> SkymapResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<Skymap>::null();
		}

		Handle<Skymap> handle = loaders_.skymapLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Skymap>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	void SkymapResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Skymap> handle = assetHandleMap_.at(assetID);
		loaders_.skymapLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	Bool SkymapResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".skymap")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".skymap");
		if (!loaders_.skymapLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

	Skymap* SkymapResource::Resolve(const Handle<Skymap>& handle)
	{
		return loaders_.skymapLoader_->Get(handle);
	}
}
