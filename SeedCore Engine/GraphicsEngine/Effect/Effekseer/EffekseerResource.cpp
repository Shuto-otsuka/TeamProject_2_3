#include <GraphicsEngine/Effect/Effekseer/EffekseerResource.h>
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
	EffekseerResource::EffekseerResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
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
	void EffekseerResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	/**
	* [EN]
	* Loads assetID unless it is already loaded and returns its handle,
	* or a null handle when the asset is unknown or cannot be created.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID がまだ読み込まれていなければ読み込み、そのハンドルを返す。
	* 未知のアセットか作れない場合は null のハンドルを返す。
	*/
	Handle<EffekseerEffect> EffekseerResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<EffekseerEffect>::null();
		}

		Handle<EffekseerEffect> handle = loaders_.effekseerLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<EffekseerEffect>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the effect loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているエフェクトを解放する。
	*/
	void EffekseerResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<EffekseerEffect> handle = assetHandleMap_.at(assetID);
		loaders_.effekseerLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Writes the ".effekseer" cache beside the ".efkefc" source
	* sourceAssetID. Nothing is written for an asset that is not a
	* ".efkefc". Returns false when the source cannot be read or created,
	* or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ".efkefc" ソース sourceAssetID の隣に ".effekseer" キャッシュを書く。
	* ".efkefc" でないアセットには何も書かない。ソースを読めない、作れない、
	* または書き込みに失敗した場合は false を返す。
	*/
	Bool EffekseerResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] Only an Effekseer source has a cache; the engine's own ".zephyr" effects share this asset type but not this format.
		/// [JP] キャッシュを持つのは Effekseer のソースだけ。エンジン独自の ".zephyr" エフェクトは同じアセット種別だが、この形式ではない。
		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		if (sourcePath.extension() != ".efkefc")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".effekseer");
		if (!loaders_.effekseerLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
	}

	/**
	* [EN]
	* Resolves a handle to an EffekseerEffect pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを EffekseerEffect ポインタに解決する。
	*/
	EffekseerEffect* EffekseerResource::Resolve(const Handle<EffekseerEffect>& handle)
	{
		return loaders_.effekseerLoader_->Get(handle);
	}
}
