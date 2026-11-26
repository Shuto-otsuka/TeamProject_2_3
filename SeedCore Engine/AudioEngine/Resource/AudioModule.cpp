#include <AudioEngine/Resource/AudioModule.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
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
	AudioModule::AudioModule() :resource_(loaders_)
	{
		/// No Code
	}

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
	void AudioModule::Install(ResourceCache& cache)
	{
		cache.Register(AssetType::Audio, resource_);
	}

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
	AudioLoaderSystem& AudioModule::Loaders()
	{
		return loaders_;
	}

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
	AudioResource& AudioModule::Resource()
	{
		return resource_;
	}
}