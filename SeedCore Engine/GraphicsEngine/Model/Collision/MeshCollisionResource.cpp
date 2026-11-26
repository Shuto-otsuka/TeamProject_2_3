#include <GraphicsEngine/Model/Collision/MeshCollisionResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	MeshCollisionResource::MeshCollisionResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource) :loaders_(loaders), modelResource_(modelResource)
	{
		/// No Code
	}

	void MeshCollisionResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	Handle<MeshCollision> MeshCollisionResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<MeshCollision>::null();
		}

		Handle<MeshCollision> handle = loaders_.meshCollisionLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<MeshCollision>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
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
		revision_++;
	}

	Bool MeshCollisionResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		Crister* crister = modelResource_.Resolve(modelResource_.Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister)
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path proxyPath = modelPath.parent_path() / (modelPath.stem().string() + ".proxy.collision");
		std::filesystem::path exactPath = modelPath.parent_path() / (modelPath.stem().string() + ".exact.collision");

		Bool allWritten = true;
		if (loaders_.meshCollisionLoader_->Bake(*crister, MeshCollisionDetail::Proxy, String(proxyPath.string())))
		{
			writtenPaths.push_back(String(proxyPath.string()));
		}
		else
		{
			allWritten = false;
		}
		if (loaders_.meshCollisionLoader_->Bake(*crister, MeshCollisionDetail::Exact, String(exactPath.string())))
		{
			writtenPaths.push_back(String(exactPath.string()));
		}
		else
		{
			allWritten = false;
		}
		return allWritten;
	}

	MeshCollision* MeshCollisionResource::Resolve(const Handle<MeshCollision>& handle)
	{
		return loaders_.meshCollisionLoader_->Get(handle);
	}
}
