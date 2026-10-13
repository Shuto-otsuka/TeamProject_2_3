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
		Load(context.cache_, assetID, 48.0f);
	}

	Handle<Font> FontResource::Load(ResourceCache& cache, Uint32 assetID, Float fontSize)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Font>::null();
		}

		Handle<Font> handle = loaders_.fontLoader_->Load(asset->fullpath_, fontSize);
		if (handle.empty())
		{
			return Handle<Font>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
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
	}

	void FontResource::Clear()
	{
		for (Handle<Font>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			loaders_.fontLoader_->Clear(handle);
		}

		assetHandleMap_.clear();
	}

	Handle<Font> FontResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Font>::null();
		}

		return assetHandleMap_.at(assetID);
	}

	Font* FontResource::Resolve(const Handle<Font>& handle)
	{
		return loaders_.fontLoader_->Get(handle);
	}

	Bool FontResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}

	void FontResource::Update(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap)
	{
		for (const Handle<Font>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			Font* font = loaders_.fontLoader_->Get(handle);
			if (!font)
			{
				continue;
			}

			font->Update();
			font->UploadAtlas(device, cmdQueue, bindlessHeap);
		}
	}
}
