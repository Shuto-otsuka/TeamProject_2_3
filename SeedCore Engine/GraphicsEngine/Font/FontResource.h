#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Font;
	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded Font handles. A font
	* keeps no cache file, so Generate is left at Asset's default and
	* writes nothing; filling and uploading the atlases is FontSystem's
	* job.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み Font ハンドル間のマッピングを管理する。
	* フォントはキャッシュファイルを持たないため、Generate は Asset の既定の
	* ままで何も書かない。アトラスを埋めてアップロードするのは FontSystem の
	* 役目。
	*/
	class FontResource :public AssetResource<Font>
	{
	private:
		friend class FontSystem;

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
		FontResource(GraphicsLoaderSystem& loaders);
		~FontResource() = default;

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
		Handle<Font> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the font loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているフォントを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Releases every font still held after a per-asset Unload pass.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット単位の Unload パスの後に、まだ保持している全フォントを
		* 解放する。
		*/
		void Clear()override;

		/**
		* [EN]
		* Resolves a handle to a Font pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを Font ポインタに解決する。
		*/
		Font* Resolve(const Handle<Font>& handle);

	private:
		/// [EN] Loaders this manager opens fonts through.
		/// [JP] このマネージャがフォントを開くのに使うローダー。
		GraphicsLoaderSystem& loaders_;
	};
}
