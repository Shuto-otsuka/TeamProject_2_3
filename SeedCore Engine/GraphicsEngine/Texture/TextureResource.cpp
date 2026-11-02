#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the loaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーを受け取る。
	*/
	TextureResource::TextureResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

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
	void TextureResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

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
	Handle<Texture> TextureResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Texture>::null();
		}

		Handle<Texture> handle = loaders_.textureLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Texture>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the texture loaded or adopted under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれた、または Adopt されたテクスチャを解放する。
	*/
	void TextureResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Texture> handle = assetHandleMap_.at(assetID);
		loaders_.textureLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

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
	Bool TextureResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".texture")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".texture");
		if (!loaders_.textureLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

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
	Texture* TextureResource::Resolve(const Handle<Texture>& handle)
	{
		return loaders_.textureLoader_->Get(handle);
	}
}
