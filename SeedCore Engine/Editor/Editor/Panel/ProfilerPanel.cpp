#include <Editor/Editor/Panel/ProfilerPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <GraphicsEngine/Graphics.h>
#include <GraphicsEngine/D3D12/Context/D3D12Adapter.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>

namespace SeedCore
{
	ProfilerPanel::ProfilerPanel(EditorContext& context) : context_(context)
	{
		/// No Code
	}

	void ProfilerPanel::Draw()
	{
		ImGuiIO& io = ImGui::GetIO();

		fpsHistory_[historyOffset_] = io.Framerate;
		frameTimeHistory_[historyOffset_] = 1000.0f / io.Framerate;
		historyOffset_ = (historyOffset_ + 1) % historySize_;

		refreshTimer_ += io.DeltaTime;
		if (refreshTimer_ >= refreshInterval_)
		{
			displayFPS_ = io.Framerate;
			displayFrameTime_ = 1000.0f / io.Framerate;
			refreshTimer_ = 0.0f;
		}

		Float availWidth = ImGui::GetContentRegionAvail().x;
		Float graphWidth = (availWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
		Float graphHeight = 80.0f;

		ImGui::SeparatorText("フレーム");

		Char fpsOverlay[32];
		snprintf(fpsOverlay, sizeof(fpsOverlay), "%.1f FPS", displayFPS_);

		ImGui::PlotLines("##FPS", fpsHistory_, historySize_, historyOffset_, fpsOverlay, 0.0f, 200.0f, ImVec2(graphWidth, graphHeight));

		ImGui::SameLine();

		Char ftOverlay[32];
		snprintf(ftOverlay, sizeof(ftOverlay), "%.2f ms", displayFrameTime_);

		ImGui::PlotLines("##FrameTime", frameTimeHistory_, historySize_, historyOffset_, ftOverlay, 0.0f, 50.0f, ImVec2(graphWidth, graphHeight));

		ImGui::SeparatorText("メモリ");

		PROCESS_MEMORY_COUNTERS_EX pmc{};
		pmc.cb = sizeof(pmc);
		if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
		{
			MEMORYSTATUSEX memStatus{};
			memStatus.dwLength = sizeof(memStatus);
			GlobalMemoryStatusEx(&memStatus);

			Float workingSetMB = static_cast<Float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
			Float totalPhysicalMB = static_cast<Float>(memStatus.ullTotalPhys) / (1024.0f * 1024.0f);
			Float cpuRatio = (totalPhysicalMB > 0.0f) ? (workingSetMB / totalPhysicalMB) : 0.0f;

			Char cpuOverlay[64];
			snprintf(cpuOverlay, sizeof(cpuOverlay), "CPU: %.1f / %.0f MB", workingSetMB, totalPhysicalMB);

			ImGui::ProgressBar(cpuRatio, ImVec2(-1.0f, 0.0f), cpuOverlay);
		}

		IDXGIAdapter4* adapter = context_.graphics_.graphics_->GetContext().GetAdapter()->Get();
		if (adapter)
		{
			DXGI_QUERY_VIDEO_MEMORY_INFO localInfo{};
			adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo);

			Float usedMB = static_cast<Float>(localInfo.CurrentUsage) / (1024.0f * 1024.0f);
			Float budgetMB = static_cast<Float>(localInfo.Budget) / (1024.0f * 1024.0f);
			Float ratio = (budgetMB > 0.0f) ? (usedMB / budgetMB) : 0.0f;

			Char vramOverlay[64];
			snprintf(vramOverlay, sizeof(vramOverlay), "VRAM: %.1f / %.1f MB", usedMB, budgetMB);

			ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f), vramOverlay);
		}

		ImGui::SeparatorText("レンダリング");

		if (ImGui::BeginTable("##RenderStats", 2))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("ドローコール");
			ImGui::TableNextColumn();
			ImGui::Text("%u", ProfilerStats::drawCallCount_);

			ImGui::EndTable();
		}
	}
}
