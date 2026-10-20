#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Model/Crister.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".material" handles.
	* Mirrors MeshCollisionResource: Load by asset ID, retrieve handles,
	* resolve to Surface pointers, unload when no longer needed. Which
	* Surface an actor's slot actually draws with is decided by
	* MaterialSystem, not here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".material" ハンドル間のマッピングを管理する。
	* MeshCollisionResource と同型: アセット ID でロード、ハンドル取得、
	* Surface ポインタへの解決、不要時のアンロード。アクターのスロットが
	* 実際にどの Surface で描かれるかは、ここではなく MaterialSystem が
	* 決める。
	*/
	class SEEDCORE_API MaterialResource :public Asset, public NonCopyable
	{
	public:
		MaterialResource(GraphicsLoaderSystem& loaders);
		~MaterialResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Surface> Load(ResourceCache& cache, Uint32 assetID);

		void Unload(Uint32 assetID)override;

		Handle<Surface> GetHandle(Uint32 assetID)const;

		Surface* Resolve(const Handle<Surface>& handle);

		Bool Contains(Uint32 assetID)const;

		Uint32 Revision()const;

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<Surface>> assetHandleMap_;

		Uint32 revision_ = 0;
	};
}
