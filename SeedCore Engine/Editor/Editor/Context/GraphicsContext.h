#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	class Graphics;
	class ImGuiRenderer;

	/**
	* [EN]
	* The renderer and the ImGui backend, owned by Engine and shared with
	* every panel.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レンダラーと ImGui のバックエンド。どちらも Engine が所有し、全パネルで
	* 共有する。
	*/
	struct GraphicsContext
	{
		/// [EN] Renderer for every view and preview.
		/// [JP] 全ビューと全プレビューのレンダラー。
		Graphics* graphics_ = nullptr;

		/// [EN] ImGui backend: docking, fonts and drawing the UI.
		/// [JP] ImGui のバックエンド。ドッキング、フォント、UI の描画を担う。
		ImGuiRenderer* imgui_ = nullptr;
	};
}
