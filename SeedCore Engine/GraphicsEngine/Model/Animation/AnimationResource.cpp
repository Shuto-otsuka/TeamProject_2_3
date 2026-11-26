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
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<Animation> AnimationResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<Animation>::null();
		}

		Handle<Animation> handle = loaders_.animationLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Animation>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
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
		revision_++;
	}

	Bool AnimationResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		AssetMeta meta;
		meta.Read(std::filesystem::path(asset->fullpath_.c_str()));
		return loaders_.animationLoader_->SplitClips(asset->fullpath_, meta.axisConvention_, writtenPaths);
	}

	Bool AnimationResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		Animation* animation = Resolve(GetHandle(assetID));
		if (!animation)
		{
			return false;
		}
		return loaders_.animationLoader_->Bake(*animation, filePath);
	}

	Animation* AnimationResource::Resolve(const Handle<Animation>& handle)
	{
		return loaders_.animationLoader_->Get(handle);
	}
}
