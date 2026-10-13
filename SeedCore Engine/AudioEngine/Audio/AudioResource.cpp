#include <AudioEngine/Audio/AudioResource.h>
#include <AudioEngine/Resource/AudioLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the resource to the loader bundle it draws its audio loader
	* from, starting with no asset loaded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声のローダーを引くためのローダーのまとまりに、このリソースを
	* 結び付ける。読み込み済みのアセットが無い状態から始まる。
	*/
	AudioResource::AudioResource(AudioLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	/**
	* [EN]
	* Loads an audio asset through the generic asset context.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 汎用アセットコンテキストを通じて音声アセットを読み込む。
	*/
	void AudioResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID);
	}


	/**
	* [EN]
	* Loads an audio asset if needed and returns its cached Sound handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 必要に応じて音声アセットを読み込み、キャッシュ済みの Sound ハンドルを返す。
	*/
	Handle<Sound> AudioResource::Load(ResourceCache& cache, Uint32 assetID)
	{
		/// [EN] Reuse the existing handle when this asset is already loaded.
		/// [JP] このアセットが読み込み済みなら、既存のハンドルを再利用する。
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		/// [EN] Resolve the asset path and load its Sound into AudioLoader storage.
		/// [JP] アセットのパスを解決し、Sound を AudioLoader のストレージへ読み込む。
		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Sound>::null();
		}

		Handle<Sound> handle = loaders_.audioLoader_->Load(asset->fullpath_);
		if (handle.empty())
		{
			return Handle<Sound>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		return handle;
	}

	/**
	* [EN]
	* Releases and removes the Sound associated with an asset ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID に対応する Sound を解放し、対応関係を削除する。
	*/
	void AudioResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		/// [EN] Release the pooled Sound before removing its asset-to-handle mapping.
		/// [JP] アセットとハンドルの対応を削除する前に、プール内の Sound を解放する。
		Handle<Sound> handle = assetHandleMap_.at(assetID);
		loaders_.audioLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
	}

	/**
	* [EN]
	* Returns the loaded Sound handle associated with an asset ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID に対応する読み込み済み Sound ハンドルを返す。
	*/
	Handle<Sound> AudioResource::GetHandle(Uint32 assetId)const
	{
		if (!assetHandleMap_.contains(assetId))
		{
			return Handle<Sound>::null();
		}

		return assetHandleMap_.at(assetId);
	}

	/**
	* [EN]
	* Resolves a Sound handle through the audio loader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* オーディオローダーを通じて Sound ハンドルを解決する。
	*/
	Sound* AudioResource::Resolve(const Handle<Sound>& handle)
	{
		return loaders_.audioLoader_->Get(handle);
	}

	/**
	* [EN]
	* Reports whether an asset ID has a loaded Sound handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID に読み込み済み Sound ハンドルがあるかを返す。
	*/
	Bool AudioResource::Contains(Uint32 assetId)const
	{
		return assetHandleMap_.contains(assetId);
	}
}
