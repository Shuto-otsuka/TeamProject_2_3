#include <GraphicsEngine/Model/Skeleton/SkeletonResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the loaders and the model manager, which owns the Crister a
	* skeleton is generated from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーと、スケルトンの生成元になる Crister を持つモデルの
	* マネージャを受け取る。
	*/
	SkeletonResource::SkeletonResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource) :loaders_(loaders), modelResource_(modelResource)
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
	void SkeletonResource::Load(const AssetContext& context, Uint32 assetID)
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
	Handle<SkeletonRig> SkeletonResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<SkeletonRig>::null();
		}

		Handle<SkeletonRig> handle = loaders_.skeletonLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<SkeletonRig>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the rig loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているリグを解放する。
	*/
	void SkeletonResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<SkeletonRig> handle = assetHandleMap_.at(assetID);
		loaders_.skeletonLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Writes "<model dir>/<model stem>.skeleton" for the skinned model
	* sourceAssetID, holding an empty rig linked to the model. An
	* existing file is left alone so the sockets a user authored survive
	* running the action again; nothing is written then. Returns false
	* when the model has no skin or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スキン付きモデル sourceAssetID 用に、モデルへ紐づいた空のリグを
	* 持つ "<モデルのディレクトリ>/<モデル名>.skeleton" を書く。既存の
	* ファイルはそのままにし、そのときは何も書かない（アクションを
	* 再実行してもユーザーが作ったソケットが残るようにするため）。モデルに
	* スキンが無い、または書き込みに失敗した場合は false を返す。
	*/
	Bool SkeletonResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] Whether the model is skinned is only known from its Crister, so the model is loaded first; this does nothing when it already is.
		/// [JP] モデルがスキン付きかどうかは Crister を見ないと分からないため、先にモデルを読み込む。読み込み済みなら何もしない。
		Crister* crister = modelResource_.Resolve(modelResource_.Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister || crister->Skins().empty())
		{
			return false;
		}

		/// [EN] The rig sits beside the model, named after it.
		/// [JP] リグはモデルの隣に、モデルと同じ名前で置く。
		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path skeletonPath = modelPath.parent_path() / (modelPath.stem().string() + ".skeleton");
		if (std::filesystem::exists(skeletonPath))
		{
			return true;
		}

		SkeletonRig rig;
		loaders_.skeletonLoader_->Populate(rig, sourceAssetID);
		if (!loaders_.skeletonLoader_->Bake(rig, String(skeletonPath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(skeletonPath.string()));
		return true;
	}

	/**
	* [EN]
	* Writes the rig loaded under assetID to filePath.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているリグを filePath へ書き出す。
	*/
	Bool SkeletonResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		SkeletonRig* rig = Resolve(GetHandle(assetID));
		if (!rig)
		{
			return false;
		}
		return loaders_.skeletonLoader_->Bake(*rig, filePath);
	}

	/**
	* [EN]
	* Resolves a handle to a SkeletonRig pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを SkeletonRig ポインタに解決する。
	*/
	SkeletonRig* SkeletonResource::Resolve(const Handle<SkeletonRig>& handle)
	{
		return loaders_.skeletonLoader_->Get(handle);
	}
}
