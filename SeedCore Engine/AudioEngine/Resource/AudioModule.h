#pragma once

#include <AudioEngine/Resource/AudioLoaderSystem.h>
#include <AudioEngine/Audio/AudioResource.h>

namespace SeedCore
{
	class ResourceCache;

	/**
	* [JP]
	* 音の読み込み係と管理係をまとめて、アセットの仕組みに登録する。
	*
	* @brief 音の仕組みをまとめる係
	*
	* [EN]
	* Groups the sound loader and manager and registers them with the asset system.
	*
	* @brief Groups the sound parts
	*/
	class SEEDCORE_API AudioModule
	{
	public:
		/**
		* [JP]
		* 読み込み係と管理係を作る。
		*
		* @brief 係を作る
		*
		* [EN]
		* Makes the loader and the manager.
		*
		* @brief Makes the parts
		*/
		AudioModule();

		/**
		* [JP]
		* 音のアセットを扱えるように、アセットの一覧に登録する。
		*
		* @brief アセットの一覧に登録する
		* @param cache 登録するアセットの一覧
		*
		* [EN]
		* Registers with the asset list so it can handle sound assets.
		*
		* @brief Registers with the asset list
		* @param cache The asset list to register with
		*/
		void Install(ResourceCache& cache);

		/**
		* [JP]
		* 読み込み係を返す。
		*
		* @brief  読み込み係を返す
		* @return 読み込み係
		*
		* [EN]
		* Returns the loaders.
		*
		* @brief  Returns the loaders
		* @return The loaders
		*/
		NODISCARD AudioLoaderSystem& Loaders();

		/**
		* [JP]
		* 管理係を返す。
		*
		* @brief  管理係を返す
		* @return 管理係
		*
		* [EN]
		* Returns the manager.
		*
		* @brief  Returns the manager
		* @return The manager
		*/
		NODISCARD AudioResource& Resource();

	private:
		/// [JP] 音の読み込み係
		/// [EN] The sound loaders
		AudioLoaderSystem loaders_;

		/// [JP] 音の管理係
		/// [EN] The sound manager
		AudioResource resource_;
	};
}