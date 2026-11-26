#pragma once
#include <Editor/Editor/Panel/Embed/CanvasViewPanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class CanvasWindowPanel
	{
	public:
		CanvasWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~CanvasWindowPanel() = default;

		void Draw();

	private:
		void DrawGizmoMenu();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		CanvasViewPanel canvasViewPanel_;
	};
}
