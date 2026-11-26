#include <Editor/Editor/Panel/Window/DiagnosticsPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>

namespace SeedCore
{
	DiagnosticsPanel::DiagnosticsPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), consolePanel_(context, imguiTexture)
	{
		/// No Code
	}

	void DiagnosticsPanel::Draw()
	{
		ImGuiID dockspaceID = context_.graphics_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		if (ImGui::Begin("診断"))
		{
			if (ImGui::BeginTabBar("##DiagnosticsTabs"))
			{
				ImGuiTabItemFlags consoleFlags = requestedTab_ == DiagnosticsTab::Console ? ImGuiTabItemFlags_SetSelected : 0;
				if (ImGui::BeginTabItem("コンソール###ConsoleTabID", nullptr, consoleFlags))
				{
					requestedTab_.reset();
					consolePanel_.Draw();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	void DiagnosticsPanel::ShowConsoleTab()
	{
		requestedTab_ = DiagnosticsTab::Console;
	}
}
