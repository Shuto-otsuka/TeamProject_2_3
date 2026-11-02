#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Texture/Texture.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded texture handles.
	* Generate writes an image's ".texture" cache. Keeping textures within
	* a memory budget is TextureStreaming's job, which reads this
	* manager's table.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済みテクスチャのハンドル間のマッピングを管理する。
	* Generate は画像の ".texture" キャッシュを書く。テクスチャをメモリ
	* 予算内に収めるのは TextureStreaming の役目で、このマネージャの
	* 対応表を読む。
	*/
	class SEEDCORE_API TextureResource :public AssetResource<Texture>
	{
	private:
		friend class TextureStreaming;

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
		TextureResource(GraphicsLoaderSystem& loaders);
		~TextureResource() = default;

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
		Handle<Texture> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the texture loaded or adopted under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれた、または Adopt されたテクスチャを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Writes the sibling ".texture" cache of the image sourceAssetID.
		* Nothing is written for an asset that already is a cache file.
		* Returns false when the image cannot be read or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 画像 sourceAssetID の隣に ".texture" キャッシュを書く。既に
		* キャッシュファイルであるアセットには何も書かない。画像を読めない、
		* または書き込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Resolves a handle to a Texture pointer without touching streaming;
		* a renderer that draws with it goes through TextureStreaming so an
		* evicted texture comes back.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ストリーミングに触れずにハンドルを Texture ポインタに解決する。
		* 描画に使う側は TextureStreaming を通し、追い出されたテクスチャが
		* 戻ってくるようにする。
		*/
		Texture* Resolve(const Handle<Texture>& handle);

	private:
		/// [EN] Loaders this manager loads textures and writes ".texture" caches through.
		/// [JP] このマネージャがテクスチャを読み込み、".texture" キャッシュを書くのに使うローダー。
		GraphicsLoaderSystem& loaders_;
	};
}
