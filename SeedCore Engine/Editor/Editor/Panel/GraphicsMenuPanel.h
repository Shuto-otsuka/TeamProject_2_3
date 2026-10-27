#pragma once
#include <GraphicsEngine/Renderer/ViewMode.h>

namespace SeedCore
{
	struct EditorContext;

	/// [JP] メインメニューバー内の「グラフィックス」メニュー。表示モード（ビューモード）
	///      の選択を EditorViewContext::viewMode_ に対して行う。状態はエディタービューの
	///      ツールバーと共有する。BeginMainMenuBar の内側で Draw を呼ぶこと。
	class GraphicsMenuPanel
	{
	public:
		GraphicsMenuPanel(EditorContext& context);
		~GraphicsMenuPanel() = default;

		void Draw();

	private:
		EditorContext& context_;
	};
}
