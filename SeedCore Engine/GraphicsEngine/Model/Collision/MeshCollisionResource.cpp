#include <GraphicsEngine/Model/Collision/MeshCollisionResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the loaders and the model manager, which owns the Crister a
	* collision mesh is baked from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーと、コリジョンメッシュの焼き出し元になる Crister を持つ
	* モデルのマネージャを受け取る。
	*/
	MeshCollisionResource::MeshCollisionResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource) :loaders_(loaders), modelResource_(modelResource)
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
	void MeshCollisionResource::Load(const AssetContext& context, Uint32 assetID)
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
	Handle<MeshCollision> MeshCollisionResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<MeshCollision>::null();
		}

		Handle<MeshCollision> handle = loaders_.meshCollisionLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<MeshCollision>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the collision mesh loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているコリジョンメッシュを解放する。
	*/
	void MeshCollisionResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<MeshCollision> handle = assetHandleMap_.at(assetID);
		loaders_.meshCollisionLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Bakes both collision meshes of the model sourceAssetID beside it,
	* overwriting any existing ones: "<model stem>.proxy.collision" from
	* its coarsest LOD and "<model stem>.exact.collision" from LOD 0.
	* Which one a collider uses is chosen where it is assigned. Returns
	* false when the model cannot be loaded or either bake failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデル sourceAssetID の2種類のコリジョンメッシュを、モデルの隣へ
	* 焼き出す（既存があれば上書き）。最も粗い LOD から
	* "<モデル名>.proxy.collision"、LOD 0 から "<モデル名>.exact.collision"。
	* コライダーがどちらを使うかは、割り当てるときに選ぶ。モデルを
	* 読み込めない、またはどちらかの焼き出しに失敗した場合は false を返す。
	*/
	Bool MeshCollisionResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] The bake reads CPU-resident cluster data off the Crister, so the model has to be loaded first; this does nothing when it already is.
		/// [JP] 焼き出しは Crister の CPU 常駐クラスタデータを読むため、先にモデルを読み込む。読み込み済みなら何もしない。
		Crister* crister = modelResource_.Resolve(modelResource_.Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister)
		{
			return false;
		}

		/// [EN] Both details are cheap to extract from a loaded Crister, so both are always written and each becomes its own asset.
		/// [JP] どちらの詳細度も読み込み済みの Crister からなら安く取り出せるため、常に両方を書き、それぞれを独立したアセットにする。
		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path proxyPath = modelPath.parent_path() / (modelPath.stem().string() + ".proxy.collision");
		std::filesystem::path exactPath = modelPath.parent_path() / (modelPath.stem().string() + ".exact.collision");

		Bool allWritten = true;
		if (loaders_.meshCollisionLoader_->Bake(*crister, MeshCollisionDetail::Proxy, String(proxyPath.string())))
		{
			writtenPaths.push_back(String(proxyPath.string()));
		}
		else
		{
			allWritten = false;
		}
		if (loaders_.meshCollisionLoader_->Bake(*crister, MeshCollisionDetail::Exact, String(exactPath.string())))
		{
			writtenPaths.push_back(String(exactPath.string()));
		}
		else
		{
			allWritten = false;
		}
		return allWritten;
	}

	/**
	* [EN]
	* Resolves a handle to a MeshCollision pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを MeshCollision ポインタに解決する。
	*/
	MeshCollision* MeshCollisionResource::Resolve(const Handle<MeshCollision>& handle)
	{
		return loaders_.meshCollisionLoader_->Get(handle);
	}
}
