#include <Editor/Editor/Panel/Embed/GraphicsMenuPanel.h>
#include <Editor/Editor/Context/EditorContext.h>

namespace SeedCore
{
	GraphicsMenuPanel::GraphicsMenuPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), viewModePanel_(context, imguiTexture)
	{
		/// No Code
	}

	void GraphicsMenuPanel::Draw()
	{
		if (ImGui::BeginMenu("グラフィックス"))
		{
			if (ImGui::BeginMenu("ビューモード"))
			{
				viewModePanel_.Draw();
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}
	}
}
