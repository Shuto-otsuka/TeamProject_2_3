#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class ConsolePanel
	{
	public:
		ConsolePanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ConsolePanel() = default;

		void Draw();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool showNotice_ = true;
		Bool showWarning_ = true;
		Bool showError_ = true;
		Bool autoScroll_ = true;
	};
}
