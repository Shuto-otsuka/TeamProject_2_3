#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/Movie/Video.h>
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
	MovieResource::MovieResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
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
	void MovieResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

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
	Handle<Video> MovieResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Video>::null();
		}

		Handle<Video> handle = loaders_.movieLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Video>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the video loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれている動画を解放する。
	*/
	void MovieResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Video> handle = assetHandleMap_.at(assetID);
		loaders_.movieLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Releases every video still held after a per-asset Unload pass.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット単位の Unload パスの後に、まだ保持している全動画を解放する。
	*/
	void MovieResource::Clear()
	{
		for (Handle<Video>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			loaders_.movieLoader_->Clear(handle);
		}
		assetHandleMap_.clear();
		revision_++;
	}

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
	Bool MovieResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".movie")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".movie");
		if (!loaders_.movieLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

	/**
	* [EN]
	* Resolves a handle to a Video pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを Video ポインタに解決する。
	*/
	Video* MovieResource::Resolve(const Handle<Video>& handle)
	{
		return loaders_.movieLoader_->Get(handle);
	}
}
