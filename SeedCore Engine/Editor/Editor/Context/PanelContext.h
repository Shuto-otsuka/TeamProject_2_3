#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	class AnimatorControllerPanel;
	class AvatarPanel;
	class BootScreenPanel;
	class ConfigPanel;
	class DiagnosticsPanel;
	class LayerSettingsPanel;
	class MaterialViewerPanel;
	class ModelTransformPanel;
	class ShortCutKeyPanel;
	class SkeletonControllerPanel;
	class SpecMemoPanel;
	class TimelinePanel;
	class TodoListPanel;
	class VersionPanel;

	/**
	* [EN]
	* Panels that other panels reach directly: the menu bar, the inspector
	* and the contents drawer open them, and the inspector lets the focused
	* tool panel draw its details. The panels are owned by Editor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ほかのパネルから直接呼ばれるパネル。メニューバー・インスペクター・
	* コンテンツドロワーが開き、インスペクターはフォーカス中のツールパネルに
	* 詳細を描かせる。パネルは Editor が所有する。
	*/
	struct PanelContext
	{
		/// [EN] Layer settings, opened from the edit menu and the inspector's layer field.
		/// [JP] レイヤーの設定。編集メニューとインスペクターのレイヤー欄から開く。
		LayerSettingsPanel* layerSettings_ = nullptr;

		/// [EN] Animator controller editor, opened from the view menu; the inspector draws its details while it is focused.
		/// [JP] アニメーターコントローラーの編集。表示メニューから開き、フォーカス中はインスペクターが詳細を描く。
		AnimatorControllerPanel* animatorController_ = nullptr;

		/// [EN] Animation timeline, opened from the view menu; the inspector draws its details while it is focused.
		/// [JP] アニメーションのタイムライン。表示メニューから開き、フォーカス中はインスペクターが詳細を描く。
		TimelinePanel* timeline_ = nullptr;

		/// [EN] Skeleton controller, opened from the view menu; the inspector draws its details while it is focused.
		/// [JP] スケルトンコントローラー。表示メニューから開き、フォーカス中はインスペクターが詳細を描く。
		SkeletonControllerPanel* skeletonController_ = nullptr;

		/// [EN] Material viewer, opened from the view menu and the contents drawer; the inspector draws its details while it is focused.
		/// [JP] マテリアルビューア。表示メニューとコンテンツドロワーから開き、フォーカス中はインスペクターが詳細を描く。
		MaterialViewerPanel* materialViewer_ = nullptr;

		/// [EN] Model transform, opened from the view menu, or on a model from the contents drawer.
		/// [JP] モデル変換。表示メニューから開くか、コンテンツドロワーからモデルを指定して開く。
		ModelTransformPanel* modelTransform_ = nullptr;

		/// [EN] Avatar generator, opened from the tools menu; the inspector draws its details while it is focused.
		/// [JP] アバター生成。ツールメニューから開き、フォーカス中はインスペクターが詳細を描く。
		AvatarPanel* avatar_ = nullptr;

		/// [EN] Boot screen editor, opened from the tools menu; the inspector draws its details while it is focused.
		/// [JP] 起動ローディング画面の編集。ツールメニューから開き、フォーカス中はインスペクターが詳細を描く。
		BootScreenPanel* bootScreen_ = nullptr;

		/// [EN] Shortcut key list, opened from the help menu.
		/// [JP] ショートカットキー一覧。ヘルプメニューから開く。
		ShortCutKeyPanel* shortCutKey_ = nullptr;

		/// [EN] Spec memo, opened from the help menu.
		/// [JP] 仕様メモ。ヘルプメニューから開く。
		SpecMemoPanel* specMemo_ = nullptr;

		/// [EN] Console and profiler tabs, each brought to the front from the help menu.
		/// [JP] コンソールとプロファイラーのタブ。ヘルプメニューからそれぞれ前面に出す。
		DiagnosticsPanel* diagnostics_ = nullptr;

		/// [EN] To-do list, opened from the help menu.
		/// [JP] ToDo リスト。ヘルプメニューから開く。
		TodoListPanel* todoList_ = nullptr;

		/// [EN] Version information, opened from the help menu.
		/// [JP] バージョン情報。ヘルプメニューから開く。
		VersionPanel* version_ = nullptr;

		/// [EN] Engine and game settings, opened from the help menu.
		/// [JP] エンジンとゲームの設定。ヘルプメニューから開く。
		ConfigPanel* config_ = nullptr;
	};
}
