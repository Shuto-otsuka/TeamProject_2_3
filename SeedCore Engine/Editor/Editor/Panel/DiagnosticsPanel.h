#pragma once
#include <Editor/Editor/Panel/BenchmarkPanel.h>
#include <Editor/Editor/Panel/ConsolePanel.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	enum class DiagnosticsTab 
	{
		Console
	};

	class DiagnosticsPanel
	{
	public:
		DiagnosticsPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~DiagnosticsPanel() = default;

		void Draw(const GpuProfiler& gpuProfiler, BenchmarkSystem& benchmark);

		void ShowConsoleTab();

		void ShowProfilerTab();

	private:
		EditorContext& context_;

		DiagnosticsTab currentTab_ = DiagnosticsTab::Console;

		Bool requestChange_ = false;

		ConsolePanel consolePanel_;

		/// [EN] CPU section and GPU pass timings, shown in their own window.
		/// [JP] CPU 区間と GPU パスの計測。独立ウィンドウに出す。
		BenchmarkPanel benchmarkPanel_;
	};
}
