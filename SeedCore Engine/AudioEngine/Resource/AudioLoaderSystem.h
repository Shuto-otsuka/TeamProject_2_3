#pragma once
#include <FoundationEngine/Prelude.h>

#include <AudioEngine/Audio/AudioLoader.h>

namespace SeedCore
{
	/**
	* [EN]
	* Bundles AudioEngine's format-specific loaders. Each loader parses its
	* file format and owns the loaded objects in a handle pool; it knows
	* nothing about asset IDs, which are mapped to handles by the matching
	* resource held by AudioModule.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* AudioEngine のフォーマット固有ローダーをまとめて保持する。各ローダーは
	* 自分のファイル形式を解析し、読み込んだオブジェクトをハンドルのプールで
	* 所有する。アセット ID は知らず、ハンドルとの対応は AudioModule が持つ
	* 対になるリソースが持つ。
	*/
	struct SEEDCORE_API AudioLoaderSystem
	{
		/// [EN] Loads ".audio" files (cue sheet or waveform) into Sound objects.
		/// [JP] ".audio" ファイル（キューシートまたは波形）を Sound へ読み込む。
		ResourcePtr<AudioLoader> audioLoader_;

		/**
		* [EN]
		* Creates every AudioEngine loader.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* AudioEngine の全ローダーを生成する。
		*/
		AudioLoaderSystem();
	};
}