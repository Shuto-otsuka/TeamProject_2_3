#pragma once
#include <Editor/Editor/Panel/Embed/ConsolePanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	enum class DiagnosticsTab
	{
		Console
	};

	class DiagnosticsPanel
	{
	public:
		DiagnosticsPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~DiagnosticsPanel() = default;

		void Draw();

		void ShowConsoleTab();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		ConsolePanel consolePanel_;

		std::optional<DiagnosticsTab> requestedTab_;
	};
}
