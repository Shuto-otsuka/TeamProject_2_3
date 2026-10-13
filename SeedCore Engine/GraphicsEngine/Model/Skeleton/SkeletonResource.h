#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".skeleton" handles.
	* Mirrors MaterialResource: Load by asset ID, retrieve handles, resolve
	* to SkeletonRig pointers, unload when no longer needed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".skeleton" ハンドル間のマッピングを管理する。
	* MaterialResource と同型: アセット ID でロード、ハンドル取得、
	* SkeletonRig ポインタへの解決、不要時のアンロード。
	*/
	class SEEDCORE_API SkeletonResource :public Asset, public NonCopyable
	{
	public:
		SkeletonResource(GraphicsLoaderSystem& loaders);
		~SkeletonResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<SkeletonRig> Load(ResourceCache& cache, Uint32 assetID);

		void Unload(Uint32 assetID)override;

		Handle<SkeletonRig> GetHandle(Uint32 assetID)const;

		SkeletonRig* Resolve(const Handle<SkeletonRig>& handle);

		Bool Contains(Uint32 assetID)const;

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<SkeletonRig>> assetHandleMap_;
	};
}
