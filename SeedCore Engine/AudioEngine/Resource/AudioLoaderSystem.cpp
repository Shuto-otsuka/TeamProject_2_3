#include <AudioEngine/Resource/AudioLoaderSystem.h>

namespace SeedCore
{
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
	AudioLoaderSystem::AudioLoaderSystem()
	{
		audioLoader_ = MakePtr<AudioLoader>();
	}
}