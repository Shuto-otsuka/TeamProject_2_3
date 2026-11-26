#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct AudioLoaderSystem;

	class Sound;
	class ResourceCache;

	/**
	* [JP]
	* アセットの番号から音を読み込んだり消したりする係。
	*
	* @brief アセットの音を管理する係
	*
	* [EN]
	* Loads and unloads sounds by their asset number.
	*
	* @brief Manages sound assets
	*/
	class SEEDCORE_API AudioResource :public AssetResource<Sound>
	{
	public:
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
		AudioResource(AudioLoaderSystem& loaders);

		~AudioResource() = default;

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
		void Load(const AssetContext& context, Uint32 assetID)override;

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
		Handle<Sound> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

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
		void Unload(Uint32 assetID)override;

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
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

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
		Sound* Resolve(const Handle<Sound>& handle);

	private:
		/// [JP] 読み込みに使う係
		/// [EN] The loaders it reads with
		AudioLoaderSystem& loaders_;
	};
}
