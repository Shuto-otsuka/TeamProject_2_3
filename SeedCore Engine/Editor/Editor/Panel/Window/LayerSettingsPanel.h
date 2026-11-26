#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class LayerSettingsPanel
	{
	public:
		LayerSettingsPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~LayerSettingsPanel() = default;

		void Draw();

		void Open();

	private:
		void DrawCollisionMatrixTab();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;
	};
}
