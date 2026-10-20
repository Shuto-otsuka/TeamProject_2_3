#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Model/Crister.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class Material;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".material" handles.
	* Mirrors MeshCollisionResource: Load by asset ID, retrieve handles,
	* resolve to Surface pointers, unload when no longer needed. Also owns
	* the per-draw resolution helper (Resolve) that picks between a Material
	* component's slot assignment and the Crister's embedded seed Surface.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".material" ハンドル間のマッピングを管理する。
	* MeshCollisionResource と同型: アセット ID でロード、ハンドル取得、
	* Surface ポインタへの解決、不要時のアンロード。描画ごとの解決ヘルパー
	* (Resolve) も持ち、Material コンポーネントのスロット割り当てと Crister
	* 内蔵のシード Surface のどちらを使うかを選ぶ。
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

		/**
		* [EN]
		* Returns the effective Surface for one SubMesh: an assigned ".material"
		* is the whole truth for the slot (textures included); a slot with
		* nothing assigned falls back to the Crister's embedded seed Surface.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1 SubMesh の実効 Surface を返す: 割り当て済みの ".material" は
		* そのスロットの全て（テクスチャ含む）。未割り当てのスロットは
		* Crister 内蔵のシード Surface へフォールバックする。
		*/
		Surface Resolve(const Crister& crister, Uint32 slot, const Material* material);

		Bool Contains(Uint32 assetID)const;

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<Surface>> assetHandleMap_;
	};
}
