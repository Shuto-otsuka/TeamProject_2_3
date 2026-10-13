#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	TextureResource::TextureResource(GraphicsLoaderSystem& loaders, BindlessHeap& heap) :loaders_(loaders), heap_(heap)
	{
		/// No Code
	}

	void TextureResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.device_, context.cmdQueue_, context.cache_, assetID);
	}

	Handle<Texture> TextureResource::Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Texture>::null();
		}

		Handle<Texture> handle = loaders_.textureLoader_->Load(device, cmdQueue, &heap_, asset->fullpath_);
		if (handle.empty())
		{
			return Handle<Texture>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	void TextureResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Texture> handle = assetHandleMap_.at(assetID);
		loaders_.textureLoader_->Clear(handle, &heap_);
		assetHandleMap_.erase(assetID);
	}

	Handle<Texture> TextureResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Texture>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	Uint32 TextureResource::GetID(Uint32 assetID)const
	{
		Handle<Texture> handle = GetHandle(assetID);
		if (handle.empty())
		{
			return SC_INVALID;
		}

		Texture* texture = loaders_.textureLoader_->Get(handle);
		if (!texture)
		{
			return SC_INVALID;
		}

		return texture->ShaderResourceViewIndex();
	}

	Uint32 TextureResource::GetID(Uint32 assetID, Uint64 frame)
	{
		Handle<Texture> handle = GetHandle(assetID);
		if (handle.empty())
		{
			return SC_INVALID;
		}

		Texture* texture = Resolve(handle, frame);
		if (!texture || !texture->Resource())
		{
			return SC_INVALID;
		}

		return texture->ShaderResourceViewIndex();
	}

	Texture* TextureResource::Resolve(const Handle<Texture>& handle, Uint64 frame)
	{
		return loaders_.textureLoader_->Resolve(&heap_, handle, frame);
	}

	Bool TextureResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}

	void TextureResource::EvictBudget(Uint64 currentFrame)
	{
		loaders_.textureLoader_->EvictBudget(&heap_, currentFrame);
	}
}
