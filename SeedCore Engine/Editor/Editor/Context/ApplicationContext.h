#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/ResourceSync.h>

namespace SeedCore
{
	/**
	* [EN]
	* Editor-wide application state shared by every panel: the shared-asset
	* sync service, the UI frame counter and the request to quit.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全パネルで共有する、エディタ全体のアプリケーション状態。共有アセットの
	* 同期サービス、UI のフレーム番号、終了の要求を持つ。
	*/
	struct ApplicationContext
	{
		/// [EN] Shared-asset sync service owned by Editor; null until Editor is constructed.
		/// [JP] Editor が所有する共有アセットの同期サービス。Editor の構築までは null。
		ResourceSync* resourceSync_ = nullptr;

		/// [EN] Number of UI frames drawn so far, advanced once per frame by Engine.
		/// [JP] これまでに描いた UI のフレーム数。Engine が毎フレーム1つ進める。
		Uint64 uiFrame_ = 0;

		/// [EN] Set when the editor should quit (window close or the menu); Engine ends its loop on it.
		/// [JP] エディタを終了すべきとき（ウィンドウを閉じた、またはメニュー）に立つ。Engine はこれでループを終える。
		Bool exitRequested_ = false;
	};
}
