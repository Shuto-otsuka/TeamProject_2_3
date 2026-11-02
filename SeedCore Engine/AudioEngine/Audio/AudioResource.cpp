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
		Load(context.cache_, assetID, context.thread_);
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
	Handle<Sound> AudioResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Sound>::null();
		}

		Handle<Sound> handle = loaders_.audioLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<Sound>::null();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the Sound loaded for an asset ID and removes the mapping.
	* Does nothing when the asset is not loaded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID に対して読み込まれた Sound を解放し、対応を削除する。
	* 読み込まれていなければ何もしない。
	*/
	void AudioResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Sound> handle = assetHandleMap_.at(assetID);
		loaders_.audioLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

	/**
	* [EN]
	* Converts the audio source sourceAssetID into the encrypted .audio
	* cache beside it, overwriting any existing one. Nothing is written
	* for an asset that already is a cache, or for a ".awb" streamed
	* alongside its ".acb". Returns false when the source cannot be
	* converted or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声素材 sourceAssetID を、その隣の暗号化された .audio キャッシュへ
	* 変換する（既存があれば上書き）。既にキャッシュであるアセットと、
	* ".acb" と一緒にストリーミングされる ".awb" には何も書かない。素材を
	* 変換できない、または書き込みに失敗した場合は false を返す。
	*/
	Bool AudioResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] A ".awb" is baked into the cache of the ".acb" beside it, so it has no cache of its own.
		/// [JP] ".awb" は隣の ".acb" のキャッシュへ一緒に焼き込まれるため、自分のキャッシュを持たない。
		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".audio" || extension == ".awb")
		{
			return true;
		}

		std::filesystem::path cachePath = sourcePath;
		cachePath.replace_extension(".audio");
		if (!loaders_.audioLoader_->Bake(asset->fullpath_, String(cachePath.string())))
		{
			return false;
		}

		writtenPaths.push_back(String(cachePath.string()));
		return true;
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
}
