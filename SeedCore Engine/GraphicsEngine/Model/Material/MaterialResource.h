#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class TextureResource;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".material" handles.
	* Mirrors MeshCollisionResource: Load by asset ID, retrieve handles,
	* resolve to Surface pointers, unload when no longer needed. Textures
	* baked into a ".material" are handed to TextureResource under the
	* asset IDs the Surface refers to them by, so the renderers resolve
	* them the same way as standalone texture assets. Which Surface an
	* actor's slot actually draws with is decided by MaterialSystem, not
	* here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".material" ハンドル間のマッピングを管理する。
	* MeshCollisionResource と同型: アセット ID でロード、ハンドル取得、
	* Surface ポインタへの解決、不要時のアンロード。".material" に焼き
	* 込まれたテクスチャは、Surface が参照するアセット ID で TextureResource
	* へ渡すので、レンダラーは単体のテクスチャアセットと同じ方法で解決
	* する。アクターのスロットが実際にどの Surface で描かれるかは、ここ
	* ではなく MaterialSystem が決める。
	*/
	class SEEDCORE_API MaterialResource :public Asset, public NonCopyable
	{
	public:
		MaterialResource(GraphicsLoaderSystem& loaders, TextureResource& textureResource);
		~MaterialResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<BakedSurface> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Handle<BakedSurface> GetHandle(Uint32 assetID)const;

		Surface* Resolve(const Handle<BakedSurface>& handle);

		Bool Contains(Uint32 assetID)const;

		Uint32 Revision()const;

	private:
		GraphicsLoaderSystem& loaders_;

		TextureResource& textureResource_;

		FlatMap<Uint32, Handle<BakedSurface>> assetHandleMap_;

		Uint32 revision_ = 0;
	};
}
