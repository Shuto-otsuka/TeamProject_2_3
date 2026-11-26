#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class ViewModePanel
	{
	public:
		ViewModePanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ViewModePanel() = default;

		void Draw();

	private:
		void DrawMode();

		void DrawFlag();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;
	};
}
