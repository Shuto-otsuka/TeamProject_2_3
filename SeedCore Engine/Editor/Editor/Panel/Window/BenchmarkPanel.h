#pragma once
#include <GraphicsEngine/Profiler/GpuProfiler.h>

namespace SeedCore
{
	struct EditorContext;
	class BenchmarkSystem;
	class ImGuiTexture;

	struct BenchmarkRow
	{
		String group_;

		String label_;

		Size groupRank_ = 0;

		Size historyIndex_ = 0;

		Double averageMilliseconds_ = 0.0;

		Double maximumMilliseconds_ = 0.0;

		Double averageCalls_ = 0.0;
	};

	class BenchmarkPanel
	{
	public:
		BenchmarkPanel(EditorContext& context, ImGuiTexture& imguiTexture);

		~BenchmarkPanel() = default;

		void Draw();

		void Open();

	private:
		void DrawFrameGraph(BenchmarkSystem& benchmark);

		void DrawHierarchy(const BenchmarkSystem& benchmark);

		void DrawTimeline(const BenchmarkSystem& benchmark);

		void DrawCpuTable(const BenchmarkSystem& benchmark);

		void DrawGpuTable(const GpuProfiler& gpuProfiler);

	private:
		SC_CONSTEX Float refreshInterval_ = 0.5f;

		SC_CONSTEX Size averageFrameCount_ = 60;

		SC_CONSTEX Float budgetMilliseconds_ = 1000.0f / 60.0f;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		DynamicArray<BenchmarkRow> cpuRows_;

		DynamicArray<GpuProfilerResult> gpuRows_;

		Float displayFrameMilliseconds_ = 0.0f;

		Float refreshTimer_ = 0.0f;

		Int selectedSlotIndex_ = -1;

		Char search_[128]{};

		Float timelineScale_ = 1.0f;

		Float timelineOffset_ = 0.0f;
	};
}
