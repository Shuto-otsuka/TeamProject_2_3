#pragma once

namespace SeedCore
{
	struct EditorContext;

	class ImGuiTexture;

	class CalculatorPanel
	{
	public:
		CalculatorPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~CalculatorPanel() = default;

		void Draw();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;
	};
}