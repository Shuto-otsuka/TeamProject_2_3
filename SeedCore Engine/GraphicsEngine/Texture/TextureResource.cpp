#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	TextureResource::TextureResource(GraphicsLoaderSystem& loaders, D3D12Context& context, BindlessHeap& heap) :loaders_(loaders), context_(context), heap_(heap)
	{
		/// No Code
	}

	void TextureResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}

	Handle<Texture> TextureResource::Load(ResourceCache& cache, Uint32 assetID)
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

		Handle<Texture> handle = loaders_.textureLoader_->Load(context_.GetDevice(), context_.GetDirectQueue(), &heap_, asset->fullpath_);
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

	void TextureResource::Adopt(Uint32 assetID, const Bitmap& bitmap, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			Handle<Texture> existing = assetHandleMap_.at(assetID);
			Texture* texture = loaders_.textureLoader_->Get(existing);
			if (texture && !texture->FilePath().view().empty())
			{
				return;
			}
			loaders_.textureLoader_->Clear(existing, &heap_);
			assetHandleMap_.erase(assetID);
		}

		Handle<Texture> handle = loaders_.textureLoader_->Adopt(context_.GetDevice(), context_.GetCopyQueue(), context_.GetLoadTransferLane(thread), &heap_, bitmap);
		if (!handle.empty())
		{
			assetHandleMap_.insert({ assetID, handle });
		}
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
