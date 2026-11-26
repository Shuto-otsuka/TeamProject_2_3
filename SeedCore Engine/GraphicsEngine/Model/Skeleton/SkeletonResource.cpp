#include <GraphicsEngine/Model/Skeleton/SkeletonResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	SkeletonResource::SkeletonResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource) :loaders_(loaders), modelResource_(modelResource)
	{
		/// No Code
	}

	void SkeletonResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<SkeletonRig> SkeletonResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<SkeletonRig>::null();
		}

		Handle<SkeletonRig> handle = loaders_.skeletonLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<SkeletonRig>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
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
		revision_++;
	}

	Bool SkeletonResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		Crister* crister = modelResource_.Resolve(modelResource_.Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister || crister->Skins().empty())
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path skeletonPath = modelPath.parent_path() / (modelPath.stem().string() + ".skeleton");
		if (std::filesystem::exists(skeletonPath))
		{
			return true;
		}

		SkeletonRig rig;
		loaders_.skeletonLoader_->Populate(rig, sourceAssetID);
		if (!loaders_.skeletonLoader_->Bake(rig, String(skeletonPath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(skeletonPath.string()));
		return true;
	}

	Bool SkeletonResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		SkeletonRig* rig = Resolve(GetHandle(assetID));
		if (!rig)
		{
			return false;
		}
		return loaders_.skeletonLoader_->Bake(*rig, filePath);
	}

	SkeletonRig* SkeletonResource::Resolve(const Handle<SkeletonRig>& handle)
	{
		return loaders_.skeletonLoader_->Get(handle);
	}
}
