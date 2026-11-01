#include <Editor/Editor/Panel/DiagnosticsPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>

namespace SeedCore
{
	DiagnosticsPanel::DiagnosticsPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), consolePanel_(imguiTexture), benchmarkPanel_(context)
	{
		/// No Code
	}

	void DiagnosticsPanel::Draw(const GpuProfiler& gpuProfiler, BenchmarkSystem& benchmark)
	{
		ImGuiID dockspaceID = context_.graphics_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		if (ImGui::Begin("診断"))
		{
			if (ImGui::BeginTabBar("##DiagnosticsTabs"))
			{
				ImGuiTabItemFlags consoleFlags = (requestChange_ && currentTab_ == DiagnosticsTab::Console) ? ImGuiTabItemFlags_SetSelected : 0;
				if (ImGui::BeginTabItem("コンソール###ConsoleTabID", nullptr, consoleFlags))
				{
					requestChange_ = false;
					consolePanel_.Draw();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();

		benchmarkPanel_.Draw(gpuProfiler, benchmark);
	}

	void DiagnosticsPanel::ShowConsoleTab()
	{
		requestChange_ = true;
		currentTab_ = DiagnosticsTab::Console;
	}

	void DiagnosticsPanel::ShowProfilerTab()
	{
		benchmarkPanel_.Open();
	}
}
