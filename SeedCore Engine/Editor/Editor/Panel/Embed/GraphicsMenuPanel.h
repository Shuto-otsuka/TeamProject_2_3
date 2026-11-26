#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <Editor/Editor/Panel/Embed/ViewModePanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class GraphicsMenuPanel
	{
	public:
		GraphicsMenuPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~GraphicsMenuPanel() = default;

		void Draw();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		ViewModePanel viewModePanel_;
	};
}
