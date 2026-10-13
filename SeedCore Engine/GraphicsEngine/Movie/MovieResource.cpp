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
		Load(context.cache_, assetID);
	}

	Handle<Video> MovieResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Video>::null();
		}

		Handle<Video> handle = loaders_.movieLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<Video>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
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
	}

	void MovieResource::Clear()
	{
		for (Handle<Video>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			loaders_.movieLoader_->Clear(handle);
		}

		assetHandleMap_.clear();
	}

	Handle<Video> MovieResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Video>::null();
		}

		return assetHandleMap_.at(assetID);
	}

	Video* MovieResource::Resolve(const Handle<Video>& handle)
	{
		return loaders_.movieLoader_->Get(handle);
	}

	Bool MovieResource::Contains(Uint32 assetId)const
	{
		return assetHandleMap_.contains(assetId);
	}

	void MovieResource::Update(ID3D12Device* device, ID3D12GraphicsCommandList6* cmdList, BindlessHeap* bindlessHeap)
	{
		for (const Handle<Video>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			Video* video = loaders_.movieLoader_->Get(handle);
			if (!video)
			{
				continue;
			}

			video->UploadFrame(device, cmdList, bindlessHeap);
		}
	}
}
