#pragma once
#include <Editor/Editor/Panel/Embed/GuizmoPanel2D.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class CanvasViewPanel
	{
	public:
		CanvasViewPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~CanvasViewPanel() = default;

		void Draw();

	private:
		void HandlePicking(const ImVec2& imageScreenPos, Float imageWidth, Float imageHeight);

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		GuizmoPanel2D guizmoPanel_;

		Bool panActive_ = false;

		Bool resetViewActive_ = false;

		Bool boxSelectPending_ = false;

		Bool boxSelectActive_ = false;

		ImVec2 boxSelectStart_ = ImVec2(0.0f, 0.0f);
	};
}
