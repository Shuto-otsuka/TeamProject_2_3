#include <GraphicsEngine/Model/Animation/AnimationResource.h>
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
	AnimationResource::AnimationResource(GraphicsLoaderSystem& loaders) :loaders_(loaders)
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
	void AnimationResource::Load(const AssetContext& context, Uint32 assetID)
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
	Handle<Animation> AnimationResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Animation>::null();
		}

		Handle<Animation> handle = loaders_.animationLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Animation>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the clip loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているクリップを解放する。
	*/
	void AnimationResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Animation> handle = assetHandleMap_.at(assetID);
		loaders_.animationLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Splits every clip embedded in the model source sourceAssetID into
	* "<model dir>/<model stem>.Animations/<clip name>.animation" files,
	* converted with the model's own axis convention. A model with no
	* clips writes nothing. Returns false when the source cannot be read
	* or a write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルのソース sourceAssetID に埋め込まれた全クリップを、モデル自身の
	* 座標系規約で変換して "<モデルのディレクトリ>/<モデル名>.Animations/
	* <クリップ名>.animation" ファイルへ分割する。クリップの無いモデルは
	* 何も書かない。ソースを読めない、または書き込みに失敗した場合は
	* false を返す。
	*/
	Bool AnimationResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] The clips are read straight from the source file, with the convention the model itself was imported with, so they drive the Crister's nodes in the same space.
		/// [JP] クリップはソースファイルから直接読み、モデル自身を取り込んだのと同じ規約で変換する。Crister のノードを同じ空間で動かすため。
		return loaders_.animationLoader_->SplitClips(asset->fullpath_, context.cache_.ReadAxisConvention(sourceAssetID), writtenPaths);
	}

	/**
	* [EN]
	* Writes the clip loaded under assetID to filePath.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているクリップを filePath へ書き出す。
	*/
	Bool AnimationResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		Animation* animation = Resolve(GetHandle(assetID));
		if (!animation)
		{
			return false;
		}
		return loaders_.animationLoader_->Bake(*animation, filePath);
	}

	/**
	* [EN]
	* Resolves a handle to an Animation pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを Animation ポインタに解決する。
	*/
	Animation* AnimationResource::Resolve(const Handle<Animation>& handle)
	{
		return loaders_.animationLoader_->Get(handle);
	}
}
