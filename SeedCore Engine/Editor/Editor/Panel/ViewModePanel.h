#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct EditorContext;

	/**
	* [EN]
	* The view mode tree of the editor view with its show flag submenu,
	* shared by the editor view's toolbar and the main menu's Graphics menu so
	* both list the same items.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタービューの表示モードのツリーと、その中の表示フラグのサブメニュー。
	* エディタービューのツールバーとメインメニューの「グラフィックス」が同じ項目を
	* 並べるよう共有する。
	*/
	class ViewModePanel
	{
	public:
		ViewModePanel(EditorContext& context);
		~ViewModePanel() = default;

		/**
		* [EN]
		* Draws the view mode tree followed by the show flag submenu, inside an
		* open popup or menu.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示モードのツリーと、その後ろに表示フラグのサブメニューを描く。開いている
		* ポップアップかメニューの内側で呼ぶこと。
		*/
		void Draw();

	private:
		/**
		* [EN]
		* Lists the basic modes at the top and every visualization category as a
		* submenu, selecting into EditorViewContext::viewMode_. The ReSTIR and
		* denoiser submenus also pick the ray-traced effect they inspect into
		* EditorViewContext::debugSource_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 基本のモードを先頭に、各ビジュアライゼーションのカテゴリをサブメニューで並べ、
		* 選んだものを EditorViewContext::viewMode_ に入れる。ReSTIR とデノイザーの
		* サブメニューでは、調べるレイトレーシングの効果も EditorViewContext::debugSource_
		* に選ぶ。
		*/
		void DrawMode();

		/**
		* [EN]
		* Lists every show flag as a check item, grouped into submenus by
		* category, toggling its bit in EditorViewContext::showFlags_. Clicking
		* an item keeps the menu open so several flags can be changed in a row.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全ての表示フラグを、カテゴリごとのサブメニューにチェック項目で並べ、
		* EditorViewContext::showFlags_ のビットを切り替える。続けていくつも
		* 切り替えられるよう、押してもメニューは閉じない。
		*/
		void DrawFlag();

	private:
		EditorContext& context_;
	};
}
