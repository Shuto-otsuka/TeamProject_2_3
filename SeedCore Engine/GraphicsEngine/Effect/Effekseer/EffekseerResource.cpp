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
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<EffekseerEffect> EffekseerResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<EffekseerEffect>::null();
		}

		Handle<EffekseerEffect> handle = loaders_.effekseerLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<EffekseerEffect>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
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
		revision_++;
	}

	Bool EffekseerResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		if (sourcePath.extension() != ".efkefc")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".effekseer");
		if (!loaders_.effekseerLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

	EffekseerEffect* EffekseerResource::Resolve(const Handle<EffekseerEffect>& handle)
	{
		return loaders_.effekseerLoader_->Get(handle);
	}
}
