#pragma once
#include <GraphicsEngine/Profiler/GpuProfiler.h>

namespace SeedCore
{
	struct EditorContext;
	class BenchmarkSystem;

	/**
	* [EN]
	* One CPU section as shown in the table, latched from its history at the
	* last refresh. A section named "Group/Name" is split into group_ and
	* label_ so sections of the same group are listed together under one
	* collapsible row.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表に出す CPU 区間1つ。最後の更新時に履歴から写し取った値を持つ。
	* "グループ/名前" の区間は group_ と label_ に分け、同じグループの区間を
	* 1つの折りたたみ行の下にまとめて並べる。
	*/
	struct BenchmarkRow
	{
		/// [EN] Part of the section name before the first '/'.
		/// [JP] 区間名の最初の '/' より前の部分。
		String group_;

		/// [EN] Part of the section name after the first '/', or the whole name when it has none.
		/// [JP] 区間名の最初の '/' より後ろの部分。'/' が無ければ名前全体。
		String label_;

		/// [EN] Order in which the group was first recorded, so groups keep the order the frame runs them in.
		/// [JP] グループが最初に記録された順番。フレーム内で走る順にグループが並ぶようにする。
		Size groupRank_ = 0;

		/// [EN] Index into BenchmarkSystem::Histories, used to plot the section's history on hover.
		/// [JP] BenchmarkSystem::Histories への添字。ホバー時に区間の履歴を描くのに使う。
		Size historyIndex_ = 0;

		/// [EN] Average time per frame over the averaged frames.
		/// [JP] 平均を取るフレーム範囲での、フレームあたりの平均時間。
		Double averageMilliseconds_ = 0.0;

		/// [EN] Worst single frame over the averaged frames.
		/// [JP] 平均を取るフレーム範囲での、最も重かった1フレームの時間。
		Double maximumMilliseconds_ = 0.0;

		/// [EN] Average number of calls per frame.
		/// [JP] フレームあたりの平均呼び出し回数。
		Double averageCalls_ = 0.0;
	};

	/**
	* [EN]
	* Shows where a frame's time goes: a budget bar for the frame and the
	* focused view's GPU time, the CPU sections recorded through Benchmark,
	* and the GPU passes recorded through GpuProfiler.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの時間がどこに使われているかを見せる。フレームとフォーカス中の
	* ビューの GPU 時間の予算バー、Benchmark で記録した CPU 区間、
	* GpuProfiler で記録した GPU パスを並べる。
	*/
	class BenchmarkPanel
	{
	public:
		/**
		* [EN]
		* Keeps the context to read which view is focused.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* どのビューにフォーカスがあるかを読むため、コンテキストを保持する。
		*/
		BenchmarkPanel(EditorContext& context);

		/**
		* [EN]
		* Destroys the panel.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パネルを破棄する。
		*/
		~BenchmarkPanel() = default;

		/**
		* [EN]
		* Refreshes the latched values when the interval has passed, then draws
		* the budget bars and both tables. Both measurement sources come in as
		* parameters, so the panel's dependency on them shows in the signature.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 間隔が経っていれば表示値を更新し、予算バーと2つの表を描く。計測元は
		* どちらも引数で受け取り、パネルがそれらに依存していることをシグネチャに出す。
		*/
		void Draw(const GpuProfiler& gpuProfiler, BenchmarkSystem& benchmark);

		/**
		* [EN]
		* Opens the stand-alone benchmark-profiler window.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 独立したベンチマークプロファイラーウィンドウを開く。
		*/
		void Open();

	private:
		/**
		* [EN]
		* Draws the CPU sections grouped by their "Group/" prefix.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CPU 区間を "グループ/" の接頭辞ごとにまとめて描く。
		*/
		void DrawFrameGraph(BenchmarkSystem& benchmark);

		void DrawHierarchy(const BenchmarkSystem& benchmark);

		void DrawTimeline(const BenchmarkSystem& benchmark);

		void DrawCpuTable(const BenchmarkSystem& benchmark);

		/**
		* [EN]
		* Draws the per-pass GPU times, one row per Stage and one column per view, with the passes that run once per frame in their own column.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パス別の GPU 時間を、Stage ごとに行、ビューごとに列を分けて描く。フレームで 1 回のパスは専用の列に出す。
		*/
		void DrawGpuTable(const GpuProfiler& gpuProfiler);

	private:
		/// [EN] Editor-wide context, read for the focused view.
		/// [JP] エディタ全体のコンテキスト。フォーカス中のビューを読むのに使う。
		EditorContext& context_;

		/// [EN] Latched CPU rows, sorted by group and then by average time.
		/// [JP] 写し取った CPU の行。グループ順、その中は平均時間の大きい順に並ぶ。
		DynamicArray<BenchmarkRow> rows_;

		/// [EN] Latched copy of the GPU time of every pass, in the order the passes ran.
		/// [JP] 全パスの GPU の時間の写し。パスが動いた順に並ぶ。
		DynamicArray<GpuProfilerResult> gpuResults_;

		/// [EN] Latched frame time, the reference for every section's share of the frame.
		/// [JP] 写し取ったフレーム時間。各区間がフレームに占める割合の基準になる。
		Float displayFrameMilliseconds_ = 0.0f;

		/// [EN] Time since the displayed values were last refreshed.
		/// [JP] 表示値を最後に更新してからの時間。
		Float refreshTimer_ = 0.0f;

		/// [EN] Seconds between refreshes; values that change every frame cannot be read.
		/// [JP] 更新の間隔（秒）。毎フレーム変わる値は読めないため間引く。
		SC_CONSTEX Float refreshInterval_ = 0.5f;

		/// [EN] Number of most recent frames the average and the maximum are taken over.
		/// [JP] 平均と最大を取る、直近のフレーム数。
		SC_CONSTEX Size averageFrameCount_ = 60;

		/// [EN] Frame budget at 60 FPS, the length a full budget bar stands for.
		/// [JP] 60 FPS でのフレーム予算。予算バーが満タンになる長さ。
		SC_CONSTEX Float budgetMilliseconds_ = 1000.0f / 60.0f;

		/// [EN] Whether the stand-alone window is shown.
		/// [JP] 独立ウィンドウを表示するかどうか。
		Bool show_ = false;

		/// [EN] Slot of the frame selected from the graph, or -1 for the newest frame.
		/// [JP] グラフから選んだフレームの枠。-1 なら最新フレーム。
		Int selectedSlot_ = -1;

		/// [EN] Filter typed above the hierarchy table.
		/// [JP] 階層表の上で入力する絞り込み文字列。
		Char search_[128]{};

		/// [EN] Horizontal scale and offset of the raw-event timeline.
		/// [JP] 生イベントタイムラインの横方向の拡大率と表示位置。
		Float timelineScale_ = 1.0f;
		Float timelineOffset_ = 0.0f;
	};
}
