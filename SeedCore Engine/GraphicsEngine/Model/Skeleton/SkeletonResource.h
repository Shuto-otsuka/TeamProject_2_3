#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class ModelResource;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".skeleton" handles:
	* Load by asset ID, retrieve handles, resolve to SkeletonRig pointers,
	* unload when no longer needed. Generate writes a model's ".skeleton"
	* file, and Bake writes an edited rig back.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".skeleton" ハンドル間のマッピングを管理する。
	* アセット ID でロード、ハンドル取得、SkeletonRig ポインタへの解決、
	* 不要時のアンロード。Generate はモデルの ".skeleton" ファイルを書き、
	* Bake は編集したリグを書き戻す。
	*/
	class SEEDCORE_API SkeletonResource :public AssetResource<SkeletonRig>
	{
	public:
		/**
		* [EN]
		* Takes the loaders and the model manager, which owns the Crister a
		* skeleton is generated from.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーと、スケルトンの生成元になる Crister を持つモデルの
		* マネージャを受け取る。
		*/
		SkeletonResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource);
		~SkeletonResource() = default;

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
		Handle<SkeletonRig> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the rig loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているリグを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Writes "<model dir>/<model stem>.skeleton" for the skinned model
		* sourceAssetID, holding an empty rig linked to the model. An
		* existing file is left alone so the sockets a user authored survive
		* running the action again; nothing is written then. Returns false
		* when the model has no skin or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スキン付きモデル sourceAssetID 用に、モデルへ紐づいた空のリグを
		* 持つ "<モデルのディレクトリ>/<モデル名>.skeleton" を書く。既存の
		* ファイルはそのままにし、そのときは何も書かない（アクションを
		* 再実行してもユーザーが作ったソケットが残るようにするため）。モデルに
		* スキンが無い、または書き込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Writes the rig loaded under assetID to filePath.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているリグを filePath へ書き出す。
		*/
		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		/**
		* [EN]
		* Resolves a handle to a SkeletonRig pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを SkeletonRig ポインタに解決する。
		*/
		SkeletonRig* Resolve(const Handle<SkeletonRig>& handle);

	private:
		/// [EN] Loaders this manager reads and writes ".skeleton" files through.
		/// [JP] このマネージャが ".skeleton" ファイルを読み書きするのに使うローダー。
		GraphicsLoaderSystem& loaders_;

		/// [EN] Model manager owning the Crister a skeleton is generated from.
		/// [JP] スケルトンの生成元になる Crister を持つモデルのマネージャ。
		ModelResource& modelResource_;
	};
}
