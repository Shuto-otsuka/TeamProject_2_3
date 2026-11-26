#pragma once
#include <Editor/Editor/Panel/Embed/GameViewPanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class GameWindowPanel
	{
	public:
		GameWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~GameWindowPanel() = default;

		void Draw();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		GameViewPanel gameViewPanel_;
	};
}
