#pragma once
#include <FoundationEngine/Prelude.h>

#include <AudioEngine/Resource/AudioLoaderSystem.h>
#include <AudioEngine/Audio/AudioResource.h>

namespace SeedCore
{
	class ResourceCache;

	/**
	* [EN]
	* AudioEngine's entry point for asset handling. Owns the engine's
	* loaders and its audio resource and registers the resource with
	* ResourceCache, so FoundationEngine drives loading through the Asset
	* interface without knowing any AudioEngine type. The application owns
	* one instance and must keep it alive until ResourceCache has been
	* destroyed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* AudioEngine のアセット処理の入口。エンジンのローダーと音声リソースを
	* 所有し、リソースを ResourceCache へ登録する。これにより FoundationEngine
	* は AudioEngine の型を一切知らずに、Asset インターフェース越しに読み込みを
	* 駆動できる。アプリケーションが1つ所有し、ResourceCache が破棄されるまで
	* 生存させておく必要がある。
	*/
	class SEEDCORE_API AudioModule
	{
	public:
		/**
		* [EN]
		* Creates the loaders, then the audio resource on top of them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーを生成し、その上に音声リソースを生成する。
		*/
		AudioModule();

		/**
		* [EN]
		* Registers the audio resource with cache under AssetType::Audio.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 音声リソースを AssetType::Audio として cache へ登録する。
		*/
		void Install(ResourceCache& cache);

		/**
		* [EN]
		* Returns the loader bundle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーのまとまりを返す。
		*/
		[[nodiscard]] AudioLoaderSystem& Loaders();

		/**
		* [EN]
		* Returns the audio resource, for typed access from AudioEngine code.
		* AudioEngine has a single resource, so it is returned directly
		* rather than through a resource bundle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 音声リソースを返す。AudioEngine のコードから型付きで使うため。
		* AudioEngine のリソースは1種類なので、まとまりを介さず直接返す。
		*/
		[[nodiscard]] AudioResource& Resource();

	private:
		/// [EN] Loaders; declared before resource_ so they are built first and destroyed last.
		/// [JP] ローダー。resource_ より前に宣言しているので、先に生成され、最後に破棄される。
		AudioLoaderSystem loaders_;

		/// [EN] Audio resource, holding a reference into loaders_.
		/// [JP] 音声リソース。loaders_ への参照を持つ。
		AudioResource resource_;
	};
}