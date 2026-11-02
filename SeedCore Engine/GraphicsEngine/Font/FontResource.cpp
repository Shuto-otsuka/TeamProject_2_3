#include <GraphicsEngine/Font/FontResource.h>
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
	FontResource::FontResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
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
	void FontResource::Load(const AssetContext& context, Uint32 assetID)
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
	Handle<Font> FontResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Font>::null();
		}

		Handle<Font> handle = loaders_.fontLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Font>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the font loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているフォントを解放する。
	*/
	void FontResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Font> handle = assetHandleMap_.at(assetID);
		loaders_.fontLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

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
	void FontResource::Clear()
	{
		for (Handle<Font>& handle : assetHandleMap_ | std::ranges::views::values)
		{
			loaders_.fontLoader_->Clear(handle);
		}
		assetHandleMap_.clear();
		revision_++;
	}

	/**
	* [EN]
	* Resolves a handle to a Font pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを Font ポインタに解決する。
	*/
	Font* FontResource::Resolve(const Handle<Font>& handle)
	{
		return loaders_.fontLoader_->Get(handle);
	}
}
