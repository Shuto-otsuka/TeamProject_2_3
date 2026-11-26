#pragma once
#include <Editor/Editor/Panel/Embed/EditorViewPanel.h>
#include <Editor/Editor/Panel/Embed/ViewModePanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class EditorWindowPanel
	{
	public:
		EditorWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~EditorWindowPanel() = default;

		void Draw();

	private:
		void DrawGuizmo();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		ViewModePanel viewModePanel_;

		EditorViewPanel editorViewPanel_;
	};
}
