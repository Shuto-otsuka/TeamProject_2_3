#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Video;
	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded Video handles.
	* Generate writes a source video's encrypted ".movie" cache; advancing
	* playback and uploading frames is MovieSystem's job.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み Video ハンドル間のマッピングを管理する。
	* Generate はソース動画の暗号化された ".movie" キャッシュを書く。再生を
	* 進めてフレームをアップロードするのは MovieSystem の役目。
	*/
	class MovieResource :public AssetResource<Video>
	{
	private:
		friend class MovieSystem;

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
		MovieResource(GraphicsLoaderSystem& loaders);
		~MovieResource() = default;

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
		* or a null handle when the asset is unknown or cannot be opened.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID がまだ読み込まれていなければ読み込み、そのハンドルを返す。
		* 未知のアセットか開けない場合は null のハンドルを返す。
		*/
		Handle<Video> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the video loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれている動画を解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Releases every video still held after a per-asset Unload pass.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット単位の Unload パスの後に、まだ保持している全動画を解放する。
		*/
		void Clear()override;

		/**
		* [EN]
		* Writes the encrypted ".movie" cache beside the source video
		* sourceAssetID. Nothing is written for an asset that already is a
		* ".movie". Returns false when the source cannot be read or the write
		* failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソース動画 sourceAssetID の隣に、暗号化された ".movie" キャッシュを
		* 書く。既に ".movie" であるアセットには何も書かない。ソースを読めない、
		* または書き込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Resolves a handle to a Video pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを Video ポインタに解決する。
		*/
		Video* Resolve(const Handle<Video>& handle);

	private:
		/// [EN] Loaders this manager opens videos and writes ".movie" caches through.
		/// [JP] このマネージャが動画を開き、".movie" キャッシュを書くのに使うローダー。
		GraphicsLoaderSystem& loaders_;
	};
}
