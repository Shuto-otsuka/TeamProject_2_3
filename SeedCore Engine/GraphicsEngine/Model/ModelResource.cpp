#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	ModelResource::ModelResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void ModelResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<Crister> ModelResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<Crister>::null();
		}

		Handle<Crister> handle = loaders_.modelLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Crister>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	void ModelResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Crister> handle = assetHandleMap_.at(assetID);
		loaders_.modelLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	Bool ModelResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		if (sourcePath.extension() == ".crister")
		{
			return true;
		}

		Crister* crister = Resolve(Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister)
		{
			return false;
		}

		std::filesystem::path cristerPath = sourcePath;
		cristerPath.replace_extension(".crister");
		if (!loaders_.modelLoader_->Bake(*crister, String(cristerPath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cristerPath.string()));
		return true;
	}

	Crister* ModelResource::Resolve(const Handle<Crister>& handle)
	{
		return loaders_.modelLoader_->Get(handle);
	}
}
