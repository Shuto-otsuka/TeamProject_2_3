#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Sky/SkymapLoader.h>
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
	SkymapResource::SkymapResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
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
	void SkymapResource::Load(const AssetContext& context, Uint32 assetID)
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
	Handle<Skymap> SkymapResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Skymap>::null();
		}

		Handle<Skymap> handle = loaders_.skymapLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Skymap>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the skymap loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているスカイマップを解放する。
	*/
	void SkymapResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Skymap> handle = assetHandleMap_.at(assetID);
		loaders_.skymapLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

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
	Bool SkymapResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".skymap")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".skymap");
		if (!loaders_.skymapLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

	/**
	* [EN]
	* Resolves a handle to a Skymap pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを Skymap ポインタに解決する。
	*/
	Skymap* SkymapResource::Resolve(const Handle<Skymap>& handle)
	{
		return loaders_.skymapLoader_->Get(handle);
	}
}
