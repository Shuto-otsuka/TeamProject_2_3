#pragma once
#include <Editor/Editor/ImGui/ImGuiTexture.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class GameViewPanel
	{
	public:
		GameViewPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~GameViewPanel() = default;

		void Draw();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;
	};
}
