#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Texture;
	class ResourceCache;
	class BindlessHeap;
	class D3D12CommandQueue;

	class SEEDCORE_API TextureResource :public Asset, public NonCopyable
	{
	public:
		TextureResource(GraphicsLoaderSystem& loaders, BindlessHeap& heap);
		~TextureResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Texture> Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, ResourceCache& cache, Uint32 assetID);

		void Unload(Uint32 assetID)override;

		Handle<Texture> GetHandle(Uint32 assetID)const;

		Uint32 GetID(Uint32 assetID)const;

		Uint32 GetID(Uint32 assetId, Uint64 frame);

		Texture* Resolve(const Handle<Texture>& handle, Uint64 frame);

		Bool Contains(Uint32 assetID)const;

		void EvictBudget(Uint64 currentFrame);

	private:
		GraphicsLoaderSystem& loaders_;

		BindlessHeap& heap_;

		FlatMap<Uint32, Handle<Texture>> assetHandleMap_;
	};
}
