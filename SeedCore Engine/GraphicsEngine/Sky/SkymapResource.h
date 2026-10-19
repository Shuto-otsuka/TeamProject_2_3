#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Skymap;
	class ResourceCache;
	class BindlessHeap;
	class D3D12CommandQueue;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded Skymap handles.
	* Mirrors the ModelResource pattern: Load by asset ID, retrieve handles,
	* resolve to Skymap pointers, and unload when no longer needed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み Skymap ハンドル間のマッピングを管理する。
	* ModelResource パターンと同じ: アセット ID でロード、ハンドル取得、
	* Skymap ポインタへの解決、不要時のアンロード。
	*/
	class SkymapResource :public Asset, public NonCopyable
	{
	public:
		SkymapResource(GraphicsLoaderSystem& loaders, BindlessHeap& heap);
		~SkymapResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Skymap> Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, ResourceCache& cache, Uint32 assetID);

		void Unload(Uint32 assetID)override;

		Handle<Skymap> GetHandle(Uint32 assetID)const;

		Skymap* Resolve(const Handle<Skymap>& handle);

		Bool Contains(Uint32 assetID)const;

	private:
		GraphicsLoaderSystem& loaders_;

		BindlessHeap& heap_;

		FlatMap<Uint32, Handle<Skymap>> assetHandleMap_;
	};
}
