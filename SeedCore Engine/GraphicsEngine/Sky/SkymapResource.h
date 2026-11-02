#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Skymap;
	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded Skymap handles.
	* Generate writes an ".hdr" source's ".skymap" cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み Skymap ハンドル間のマッピングを管理する。
	* Generate は ".hdr" ソースの ".skymap" キャッシュを書く。
	*/
	class SkymapResource :public AssetResource<Skymap>
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
		SkymapResource(GraphicsLoaderSystem& loaders);
		~SkymapResource() = default;

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
		Handle<Skymap> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the skymap loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているスカイマップを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Writes the ".skymap" cache beside the ".hdr" source sourceAssetID.
		* Nothing is written for an asset that already is a ".skymap".
		* Returns false when the source cannot be decoded or the write
		* failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ".hdr" ソース sourceAssetID の隣に ".skymap" キャッシュを書く。既に
		* ".skymap" であるアセットには何も書かない。ソースをデコードできない、
		* または書き込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Resolves a handle to a Skymap pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを Skymap ポインタに解決する。
		*/
		Skymap* Resolve(const Handle<Skymap>& handle);

	private:
		/// [EN] Loaders this manager reads skymaps and writes ".skymap" caches through.
		/// [JP] このマネージャがスカイマップを読み込み、".skymap" キャッシュを書くのに使うローダー。
		GraphicsLoaderSystem& loaders_;
	};
}
