#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <AudioEngine/Audio/Sound.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音のファイルを読み込んで、使える音にする係。
	*
	* @brief 音のファイルを読み込む係
	*
	* [EN]
	* Reads sound files and turns them into sounds you can use.
	*
	* @brief Reads sound files
	*/
	class SEEDCORE_API AudioLoader :public AssetLoader<Sound>
	{
	private:
		friend class AudioResource;

	public:
		/**
		* [JP]
		* 音のファイルを読む準備をする。
		*
		* @brief 読む準備をする
		*
		* [EN]
		* Gets ready to read sound files.
		*
		* @brief Gets ready to read
		*/
		AudioLoader();

		/**
		* [JP]
		* 読み込みに使ったものを片付ける。
		*
		* @brief 片付ける
		*
		* [EN]
		* Cleans up what was used for reading.
		*
		* @brief Cleans up
		*/
		~AudioLoader();

		/**
		* [JP]
		* ファイルを読み込んで音を作る。
		*
		* @brief  ファイルを読み込む
		* @param  filePath 読むファイル
		* @param  thread   読み込むスレッド
		* @return 作った音のハンドル。失敗したら空
		*
		* [EN]
		* Reads a file and makes a sound.
		*
		* @brief  Reads a file
		* @param  filePath The file to read
		* @param  thread   The thread to read on
		* @return A handle to the new sound, or empty on failure
		*/
		Handle<Sound> Load(String filePath, LoadThread thread);

		/**
		* [JP]
		* 読み込んだ音を消す。
		*
		* @brief 音を消す
		* @param handle 消す音のハンドル
		*
		* [EN]
		* Throws away a loaded sound.
		*
		* @brief Removes a sound
		* @param handle The handle of the sound to remove
		*/
		void Clear(Handle<Sound>& handle)noexcept;

	private:
		/**
		* [JP]
		* 元のファイルを、すぐ読める形（キャッシュ）にして保存する。
		*
		* @brief  キャッシュを作る
		* @param  sourcePath 元のファイル
		* @param  cachePath  保存する場所
		* @return うまくいったら true
		*
		* [EN]
		* Saves the original file in a quick-to-read form (a cache).
		*
		* @brief  Makes a cache
		* @param  sourcePath The original file
		* @param  cachePath  Where to save it
		* @return True if it worked
		*/
		Bool Bake(String sourcePath, String cachePath);

	private:
		/// [JP] COM の準備をこちらでしたか
		/// [EN] Whether this loader set up COM itself
		Bool ownsComInitialize_ = false;

		/// [JP] 音を変換する仕組み（Media Foundation）を動かしたか
		/// [EN] Whether the sound converter (Media Foundation) was started
		Bool mfStarted_ = false;
	};
}
