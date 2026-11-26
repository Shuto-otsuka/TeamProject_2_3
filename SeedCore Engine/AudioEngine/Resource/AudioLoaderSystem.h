#pragma once

#include <AudioEngine/Audio/AudioLoader.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音の読み込み係をまとめて持つ入れ物。
	*
	* @brief 音の読み込み係の入れ物
	*
	* [EN]
	* Holds the sound loaders together.
	*
	* @brief Holds the sound loaders
	*/
	struct SEEDCORE_API AudioLoaderSystem
	{
		/// [JP] 音の読み込み係
		/// [EN] The sound loader
		ResourcePtr<AudioLoader> audioLoader_;

		/**
		* [JP]
		* 読み込み係を作る。
		*
		* @brief 読み込み係を作る
		*
		* [EN]
		* Makes the loaders.
		*
		* @brief Makes the loaders
		*/
		AudioLoaderSystem();
	};
}