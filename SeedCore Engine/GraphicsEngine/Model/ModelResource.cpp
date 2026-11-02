#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
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
	ModelResource::ModelResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
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
	void ModelResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	/**
	* [EN]
	* Loads a model by asset ID. If already loaded, returns the existing
	* handle. thread selects the transfer lane the upload is recorded on,
	* so a load on the async worker and one on the main thread never
	* share a lane.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID でモデルをロードする。既にロード済みなら既存のハンドルを
	* 返す。thread でアップロードを記録する転送レーンを選び、非同期の
	* ワーカーとメインスレッドの読み込みが同じレーンを使わないようにする。
	*/
	Handle<Crister> ModelResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Crister>::null();
		}

		Handle<Crister> handle = loaders_.modelLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Crister>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the model loaded under assetID, GPU residency included.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているモデルを、GPU 常駐分も含めて解放する。
	*/
	void ModelResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Crister> handle = assetHandleMap_.at(assetID);
		loaders_.modelLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Writes the ".crister" cache beside the model source sourceAssetID,
	* overwriting any existing one, from the model as it is baked in
	* memory. Nothing is written for an asset that already is a
	* ".crister". Returns false when the model cannot be loaded or the
	* write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルのソース sourceAssetID の隣に、メモリ上で焼き込まれた状態の
	* モデルから ".crister" キャッシュを書く（既存があれば上書き）。既に
	* ".crister" であるアセットには何も書かない。モデルを読み込めない、
	* または書き込みに失敗した場合は false を返す。
	*/
	Bool ModelResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		if (sourcePath.extension() == ".crister")
		{
			return true;
		}

		/// [EN] Loading a source bakes it in memory exactly as the cache stores it, so the loaded model is what gets written; this does nothing when it is already loaded.
		/// [JP] ソースを読み込むと、キャッシュが持つのと同じ形にメモリ上で焼き込まれるため、読み込んだモデルをそのまま書き出す。読み込み済みなら何もしない。
		Crister* crister = Resolve(Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister)
		{
			return false;
		}

		std::filesystem::path cristerPath = sourcePath;
		cristerPath.replace_extension(".crister");
		if (!loaders_.modelLoader_->Bake(*crister, String(cristerPath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cristerPath.string()));
		return true;
	}

	/**
	* [EN]
	* Resolves a handle to a Crister pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを Crister ポインタに解決する。
	*/
	Crister* ModelResource::Resolve(const Handle<Crister>& handle)
	{
		return loaders_.modelLoader_->Get(handle);
	}
}
