#include <Editor/Editor/Panel/GraphicsMenuPanel.h>
#include <Editor/Editor/Context/EditorContext.h>

namespace SeedCore
{
	GraphicsMenuPanel::GraphicsMenuPanel(EditorContext& context) : context_(context), viewModePanel_(context)
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
