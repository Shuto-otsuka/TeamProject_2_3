#pragma once
#include <Editor/Editor/Panel/Embed/GameViewPanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class FullscreenPanel
	{
	public:
		FullscreenPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~FullscreenPanel() = default;

		void Draw();

		void Open();

		NODISCARD Bool Opened()const;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		GameViewPanel gameViewPanel_;

		Bool open_ = false;
	};
}
