#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Sky/SkymapLoader.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	SkymapResource::SkymapResource(GraphicsLoaderSystem& loaders, BindlessHeap& heap) :loaders_(loaders), heap_(heap)
	{
		/// No Code
	}

	void SkymapResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.device_, context.cmdQueue_, context.cache_, assetID);
	}

	Handle<Skymap> SkymapResource::Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Skymap>::null();
		}

		Handle<Skymap> handle = loaders_.skymapLoader_->Load(device, cmdQueue, &heap_, asset->fullpath_);
		if (handle.empty())
		{
			return Handle<Skymap>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	void SkymapResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Skymap> handle = assetHandleMap_.at(assetID);
		loaders_.skymapLoader_->Clear(handle, &heap_);
		assetHandleMap_.erase(assetID);
	}

	Handle<Skymap> SkymapResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Skymap>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	Skymap* SkymapResource::Resolve(const Handle<Skymap>& handle)
	{
		return loaders_.skymapLoader_->Get(handle);
	}

	Bool SkymapResource::Contains(Uint32 assetId)const
	{
		return assetHandleMap_.contains(assetId);
	}
}
