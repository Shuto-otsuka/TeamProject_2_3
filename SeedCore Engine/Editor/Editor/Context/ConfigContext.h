#pragma once
#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>

namespace SeedCore
{
	/**
	* [EN]
	* Settings files shared by Engine and the config panel: the game's
	* settings (window, output resolution, upscaling, frame generation,
	* start scene and so on) and the editor's own settings (camera, font
	* size, audio ACF and so on), plus the requests to rebuild rendering
	* after a game setting changes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Engine と設定パネルで共有する設定ファイル。ゲームの設定（ウィンドウ、
	* 出力解像度、アップスケール、フレーム生成、起動シーンなど）と、エディタ
	* 自身の設定（カメラ、文字サイズ、オーディオの ACF など）を持つ。ゲームの
	* 設定が変わったあとに描画を作り直す依頼も持つ。
	*/
	struct ConfigContext
	{
		/// [EN] The game's settings owned by Engine, loaded once at startup and saved by the config panel on every change.
		/// [JP] Engine が所有するゲームの設定。起動時に1回読み込み、設定パネルが変更のたびに保存する。
		GameConfig* game_ = nullptr;

		/// [EN] The editor's own settings owned by Engine, loaded once at startup, saved by the config panel on every change and by Engine on exit with the current camera, font size and last scene.
		/// [JP] Engine が所有するエディタ自身の設定。起動時に1回読み込み、設定パネルが変更のたびに保存し、終了時には Engine が今のカメラ・文字サイズ・最後に開いたシーンを入れて保存する。
		EditorConfig* editor_ = nullptr;
	};
}
