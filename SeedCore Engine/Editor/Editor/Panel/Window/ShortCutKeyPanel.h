#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class ShortCutKeyPanel
	{
	public:
		ShortCutKeyPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ShortCutKeyPanel() = default;

		void Draw();

		void Open();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;
	};
}
