#include <AudioEngine/Audio/AudioResource.h>
#include <AudioEngine/Resource/AudioLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [JP]
	* 読み込みに使う係を受け取る。
	*
	* @brief 読み込み係を受け取る
	* @param loaders 読み込みに使う係
	*
	* [EN]
	* Takes the loaders it reads with.
	*
	* @brief Takes the loaders
	* @param loaders The loaders to read with
	*/
	AudioResource::AudioResource(AudioLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	/**
	* [JP]
	* アセットの番号の音を読み込む。
	*
	* @brief 音を読み込む
	* @param context 読み込みに使う情報
	* @param assetID 読み込むアセットの番号
	*
	* [EN]
	* Loads the sound with this asset number.
	*
	* @brief Loads a sound
	* @param context What to load with
	* @param assetID The asset number to load
	*/
	void AudioResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

	/**
	* [JP]
	* アセットの番号の音を読み込む。もう読み込んでいたらそれを返す。
	*
	* @brief  音を読み込む
	* @param  cache   アセットの一覧
	* @param  assetID 読み込むアセットの番号
	* @param  thread  読み込むスレッド
	* @return 音のハンドル。失敗したら空
	*
	* [EN]
	* Loads the sound with this asset number, or returns it if already loaded.
	*
	* @brief  Loads a sound
	* @param  cache   The asset list
	* @param  assetID The asset number to load
	* @param  thread  The thread to load on
	* @return A handle to the sound, or empty on failure
	*/
	Handle<Sound> AudioResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		/// [JP] もう読み込んでいれば、それを返す
		/// [EN] If it is already loaded, return that
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<Sound>::null();
		}

		/// [JP] ファイルを読み込んで、番号と一緒に覚えておく
		/// [EN] Read the file and remember it with its number
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
	* [JP]
	* 読み込んだ音を消す。
	*
	* @brief 音を消す
	* @param assetID 消すアセットの番号
	*
	* [EN]
	* Unloads a sound.
	*
	* @brief Unloads a sound
	* @param assetID The asset number to unload
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
	* [JP]
	* 元の音のファイルから、すぐ読めるキャッシュを作る。
	*
	* @brief  キャッシュを作る
	* @param  context       使う情報
	* @param  sourceAssetID 元のアセットの番号
	* @param  writtenPaths  書いたファイルの場所が入る
	* @return うまくいったら true
	*
	* [EN]
	* Makes a quick-to-read cache from the original sound file.
	*
	* @brief  Makes a cache
	* @param  context       What to use
	* @param  sourceAssetID The original asset number
	* @param  writtenPaths  Gets the files that were written
	* @return True if it worked
	*/
	Bool AudioResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.Record(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [JP] もうキャッシュの形のファイルなら、作る必要はない
		/// [EN] If the file is already a cache, there is nothing to make
		std::filesystem::path sourcePath(asset->fullpath_.c_str());
		std::string extension = sourcePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });
		if (extension == ".audio" || extension == ".awb")
		{
			return true;
		}

		/// [JP] 同じ場所に .audio のキャッシュを作る
		/// [EN] Make a .audio cache next to it
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
	* [JP]
	* ハンドルから音そのものを取り出す。
	*
	* @brief  音を取り出す
	* @param  handle 取り出すハンドル
	* @return 音。なければ nullptr
	*
	* [EN]
	* Gets the sound itself from a handle.
	*
	* @brief  Gets the sound
	* @param  handle The handle to look up
	* @return The sound, or nullptr if there is none
	*/
	Sound* AudioResource::Resolve(const Handle<Sound>& handle)
	{
		return loaders_.audioLoader_->Get(handle);
	}
}
