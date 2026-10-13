#include <AudioEngine/Resource/AudioLoaderSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates every AudioEngine loader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* AudioEngine の全ローダーを生成する。
	*/
	AudioLoaderSystem::AudioLoaderSystem()
	{
		audioLoader_ = MakePtr<AudioLoader>();
	}
}