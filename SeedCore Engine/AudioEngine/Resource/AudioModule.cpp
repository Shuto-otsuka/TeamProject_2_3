#include <AudioEngine/Resource/AudioModule.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the loaders, then the audio resource on top of them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーを生成し、その上に音声リソースを生成する。
	*/
	AudioModule::AudioModule() :resource_(loaders_)
	{
		/// No Code
	}

	/**
	* [EN]
	* Registers the audio resource with cache under AssetType::Audio.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声リソースを AssetType::Audio として cache へ登録する。
	*/
	void AudioModule::Install(ResourceCache& cache)
	{
		cache.Register(AssetType::Audio, resource_);
	}

	/**
	* [EN]
	* Returns the loader bundle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーのまとまりを返す。
	*/
	AudioLoaderSystem& AudioModule::Loaders()
	{
		return loaders_;
	}

	/**
	* [EN]
	* Returns the audio resource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声リソースを返す。
	*/
	AudioResource& AudioModule::Resource()
	{
		return resource_;
	}
}