#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Video;
	class BindlessHeap;
	class ResourceCache;

	class MovieResource :public Asset, public NonCopyable
	{
	public:
		MovieResource(GraphicsLoaderSystem& loaders);
		~MovieResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Video> Load(ResourceCache& cache, Uint32 assetID);

		void Unload(Uint32 assetID)override;

		void Clear()override;

		Handle<Video> GetHandle(Uint32 assetID)const;

		Video* Resolve(const Handle<Video>& handle);

		Bool Contains(Uint32 assetID)const;

		void Update(ID3D12Device* device, ID3D12GraphicsCommandList6* cmdList, BindlessHeap* bindlessHeap);

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<Video>> assetHandleMap_;
	};
}
