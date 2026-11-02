#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class ModelResource;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".collision"
	* handles. Generate bakes both collision meshes of a model.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".collision" ハンドル間のマッピングを管理
	* する。Generate はモデルの2種類のコリジョンメッシュを両方焼き出す。
	*/
	class SEEDCORE_API MeshCollisionResource :public AssetResource<MeshCollision>
	{
	public:
		/**
		* [EN]
		* Takes the loaders and the model manager, which owns the Crister a
		* collision mesh is baked from.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーと、コリジョンメッシュの焼き出し元になる Crister を持つ
		* モデルのマネージャを受け取る。
		*/
		MeshCollisionResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource);
		~MeshCollisionResource() = default;

		/**
		* [EN]
		* Loads the asset registered under assetID (the entry point
		* ResourceCache drives).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で登録されているアセットを読み込む（ResourceCache が動かす
		* 入口）。
		*/
		void Load(const AssetContext& context, Uint32 assetID)override;

		/**
		* [EN]
		* Loads assetID unless it is already loaded and returns its handle,
		* or a null handle when the asset is unknown or cannot be read.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID がまだ読み込まれていなければ読み込み、そのハンドルを返す。
		* 未知のアセットか読めない場合は null のハンドルを返す。
		*/
		Handle<MeshCollision> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the collision mesh loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているコリジョンメッシュを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Bakes both collision meshes of the model sourceAssetID beside it,
		* overwriting any existing ones: "<model stem>.proxy.collision" from
		* its coarsest LOD and "<model stem>.exact.collision" from LOD 0.
		* Which one a collider uses is chosen where it is assigned. Returns
		* false when the model cannot be loaded or either bake failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデル sourceAssetID の2種類のコリジョンメッシュを、モデルの隣へ
		* 焼き出す（既存があれば上書き）。最も粗い LOD から
		* "<モデル名>.proxy.collision"、LOD 0 から "<モデル名>.exact.collision"。
		* コライダーがどちらを使うかは、割り当てるときに選ぶ。モデルを
		* 読み込めない、またはどちらかの焼き出しに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Resolves a handle to a MeshCollision pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを MeshCollision ポインタに解決する。
		*/
		MeshCollision* Resolve(const Handle<MeshCollision>& handle);

	private:
		/// [EN] Loaders this manager reads and bakes ".collision" files through.
		/// [JP] このマネージャが ".collision" ファイルを読み込み、焼き出すのに使うローダー。
		GraphicsLoaderSystem& loaders_;

		/// [EN] Model manager owning the Crister a collision mesh is baked from.
		/// [JP] コリジョンメッシュの焼き出し元になる Crister を持つモデルのマネージャ。
		ModelResource& modelResource_;
	};
}
