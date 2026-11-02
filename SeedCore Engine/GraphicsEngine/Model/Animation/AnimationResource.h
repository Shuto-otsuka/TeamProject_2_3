#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Animation/Animation.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".animation"
	* handles. Generate splits a model's embedded clips into ".animation"
	* files, and Bake writes an edited clip back.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".animation" ハンドル間のマッピングを管理
	* する。Generate はモデルに埋め込まれたクリップを ".animation" ファイル
	* へ分割し、Bake は編集したクリップを書き戻す。
	*/
	class SEEDCORE_API AnimationResource :public AssetResource<Animation>
	{
	public:
		/**
		* [EN]
		* Takes the loaders.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーを受け取る。
		*/
		AnimationResource(GraphicsLoaderSystem& loaders);
		~AnimationResource() = default;

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
		Handle<Animation> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the clip loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているクリップを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Splits every clip embedded in the model source sourceAssetID into
		* "<model dir>/<model stem>.Animations/<clip name>.animation" files,
		* converted with the model's own axis convention. A model with no
		* clips writes nothing. Returns false when the source cannot be read
		* or a write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデルのソース sourceAssetID に埋め込まれた全クリップを、モデル自身の
		* 座標系規約で変換して "<モデルのディレクトリ>/<モデル名>.Animations/
		* <クリップ名>.animation" ファイルへ分割する。クリップの無いモデルは
		* 何も書かない。ソースを読めない、または書き込みに失敗した場合は
		* false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Writes the clip loaded under assetID to filePath.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているクリップを filePath へ書き出す。
		*/
		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		/**
		* [EN]
		* Resolves a handle to an Animation pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを Animation ポインタに解決する。
		*/
		Animation* Resolve(const Handle<Animation>& handle);

	private:
		/// [EN] Loaders this manager reads, splits and writes clips through.
		/// [JP] このマネージャがクリップを読み込み、分割し、書き出すのに使うローダー。
		GraphicsLoaderSystem& loaders_;
	};
}
