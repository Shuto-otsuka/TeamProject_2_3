#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Editor clock: accumulates real (unscaled, always-running) delta and
	* total time each frame, independent of the game's play/pause state
	* and time scale. It drives the editor view, canvas and preview
	* rendering, which keep animating while the game is stopped or paused.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタ用のクロック。実時間（タイムスケール未適用・常時進行）の
	* デルタと合計時間を毎フレーム累積し、ゲームの再生/一時停止状態や
	* タイムスケールの影響を受けない。ゲームが停止中や一時停止中でも動き
	* 続けるエディタビュー・キャンバス・プレビューの描画に使う。
	*/
	class SEEDCORE_API WorldTimer
	{
	public:
		/**
		* [EN]
		* Advances one frame by delta seconds, accumulating into total_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* delta 秒ぶん1フレーム進め、total_ に累積する。
		*/
		void Tick(Float delta);

		/**
		* [EN]
		* Returns the delta time passed to the last Tick().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近の Tick() に渡されたデルタタイムを返す。
		*/
		Float DeltaTime()const;

		/**
		* [EN]
		* Returns the total time accumulated since engine start.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンジン起動からの累積合計時間を返す。
		*/
		Float TotalTime()const;

	private:
		/// [EN] Delta time passed to the last Tick().
		/// [JP] 直近の Tick() に渡されたデルタタイム。
		Float delta_ = 0.0f;

		/// [EN] Total time accumulated since engine start.
		/// [JP] エンジン起動からの累積合計時間。
		Double total_ = 0.0;
	};
}
