#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/Command/History.h>
#include <FoundationEngine/File/FilePath.h>

namespace SeedCore
{
	/**
	* [EN]
	* The scene being edited: which file it is, a request to open another
	* one, and the undo/redo history of edits made to it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 編集中のシーン。どのファイルか、別のシーンを開く依頼、そのシーンへの
	* 編集の元に戻す・やり直しの履歴を持つ。
	*/
	struct SceneContext
	{
		/// [EN] File of the scene being edited, held with the project root so its relative path and file name are at hand; empty for a new scene not yet saved.
		/// [JP] 編集中のシーンのファイル。相対パスやファイル名もすぐ取れるよう、プロジェクトのルートと一緒に持つ。まだ保存していない新規シーンでは空。
		FilePath path_;

		/// [EN] Asset ID of a scene another panel asked to open, picked up and cleared by the menu bar; 0 means no request.
		/// [JP] 他のパネルが開くよう頼んだシーンのアセット ID。メニューバーが受け取って 0 に戻す。0 は依頼なし。
		Uint32 request_ = 0;

		/// [EN] Undo/redo history of edits to the scene.
		/// [JP] シーンへの編集の、元に戻す・やり直しの履歴。
		History history_;
	};
}
