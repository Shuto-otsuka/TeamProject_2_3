#include <Editor/Editor/Panel/BenchmarkPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <GraphicsEngine/Graphics.h>
#include <GraphicsEngine/D3D12/Context/D3D12Adapter.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>

namespace SeedCore
{
	/**
	* [EN]
	* Keeps the context to read which view is focused.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* どのビューにフォーカスがあるかを読むため、コンテキストを保持する。
	*/
	BenchmarkPanel::BenchmarkPanel(EditorContext& context) : context_(context)
	{
		/// No Code
	}

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
	void BenchmarkPanel::Draw(const GpuProfiler& gpuProfiler, BenchmarkSystem& benchmark)
	{
		if (!show_)
		{
			return;
		}

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 size = ImVec2(viewport->WorkSize.x * 0.92f, viewport->WorkSize.y * 0.92f);
		ImVec2 position = ImVec2(viewport->WorkPos.x + (viewport->WorkSize.x - size.x) * 0.5f, viewport->WorkPos.y + (viewport->WorkSize.y - size.y) * 0.5f);
		ImGui::SetNextWindowPos(position, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
		if (!ImGui::Begin("ベンチマークプロファイラー", &show_))
		{
			ImGui::End();
			return;
		}

		ImGuiIO& io = ImGui::GetIO();

		const Uint32 viewCount = static_cast<Uint32>(GpuProfileView::Count);
		const Uint32 scopeCount = static_cast<Uint32>(GpuProfileScope::Count);

		/// [EN] The first draw refreshes at once, so the panel does not open empty for half a second.
		/// [JP] 最初の描画ではすぐに更新し、開いてから0.5秒間パネルが空にならないようにする。
		refreshTimer_ += io.DeltaTime;
		if (refreshTimer_ >= refreshInterval_ || rows_.empty())
		{
			refreshTimer_ = 0.0f;
			displayFrameMilliseconds_ = io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f;

			/// [EN] Groups in the order they first appear in the histories, which is the order the frame records them in.
			/// [JP] 履歴に最初に現れた順のグループ。フレームが記録する順と同じになる。
			DynamicArray<String> groups;

			rows_.clear();
			std::span<const BenchmarkHistory> histories = benchmark.Histories();
			for (Size historyIndex = 0; historyIndex < histories.size(); historyIndex++)
			{
				const BenchmarkHistory& history = histories[historyIndex];

				/// [EN] Only the most recent frames count, and never more than the ring actually holds.
				/// [JP] 数えるのは直近のフレームだけで、リングに実際に残っている数を超えない。
				Size ringSize = history.seconds_.size();
				Size frameCount = Min(Min(history.count_, ringSize), averageFrameCount_);
				if (frameCount == 0)
				{
					continue;
				}

				/// [EN] Walk the ring backwards from the newest sample.
				/// [JP] 最新のサンプルからリングを逆向きにたどる。
				Double totalSeconds = 0.0;
				Double maximumSeconds = 0.0;
				Double totalCalls = 0.0;
				for (Size frameIndex = 0; frameIndex < frameCount; frameIndex++)
				{
					Size slot = (history.next_ + ringSize - 1 - frameIndex) % ringSize;
					totalSeconds += history.seconds_[slot];
					maximumSeconds = Max(maximumSeconds, history.seconds_[slot]);
					totalCalls += static_cast<Double>(history.calls_[slot]);
				}

				BenchmarkRow row;
				row.historyIndex_ = historyIndex;
				row.averageMilliseconds_ = totalSeconds / static_cast<Double>(frameCount) * 1000.0;
				row.maximumMilliseconds_ = maximumSeconds * 1000.0;
				row.averageCalls_ = totalCalls / static_cast<Double>(frameCount);

				/// [EN] "Group/Name" splits at the first '/'; a name without one goes into a catch-all group.
				/// [JP] "グループ/名前" は最初の '/' で分ける。'/' の無い名前はまとめ用のグループに入れる。
				std::string_view name = static_cast<std::string_view>(history.name_);
				Size separator = name.find('/');
				if (separator == std::string_view::npos)
				{
					row.group_ = String("その他");
					row.label_ = history.name_;
				}
				else
				{
					row.group_ = String(name.substr(0, separator));
					row.label_ = String(name.substr(separator + 1));
				}

				auto groupIt = std::ranges::find(groups, row.group_);
				if (groupIt == groups.end())
				{
					groups.push_back(row.group_);
					row.groupRank_ = groups.size() - 1;
				}
				else
				{
					row.groupRank_ = static_cast<Size>(groupIt - groups.begin());
				}

				rows_.push_back(row);
			}

			/// [EN] Groups stay in frame order; within a group the heaviest section comes first.
			/// [JP] グループはフレームの順のまま、グループ内は重い区間を先にする。
			std::ranges::stable_sort(rows_, [](const BenchmarkRow& left, const BenchmarkRow& right)
				{
					if (left.groupRank_ != right.groupRank_)
					{
						return left.groupRank_ < right.groupRank_;
					}
					return left.averageMilliseconds_ > right.averageMilliseconds_;
				});

			for (Uint32 viewIndex = 0; viewIndex < viewCount; viewIndex++)
			{
				for (Uint32 scopeIndex = 0; scopeIndex < scopeCount; scopeIndex++)
				{
					gpuDisplay_[viewIndex][scopeIndex] = gpuProfiler.GetMilliseconds(static_cast<GpuProfileView>(viewIndex), static_cast<GpuProfileScope>(scopeIndex));
				}
			}
		}

		if (ImGui::Button(benchmark.Paused() ? "再開" : "一時停止"))
		{
			benchmark.Paused(!benchmark.Paused());
		}
		ImGui::SameLine();
		ImGui::TextDisabled("予算: 60 / 30 FPS");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(180.0f);
		ImGui::InputTextWithHint("##BenchmarkSearch", "検索", search_, sizeof(search_));
		ImGui::SameLine();
		Float frameMilliseconds = io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f;
		Uint32 focusedViewIndex = static_cast<Uint32>(context_.view_.focused_);
		Float gpuMilliseconds = 0.0f;
		for (Uint32 scopeIndex = 0; scopeIndex < scopeCount; scopeIndex++)
		{
			gpuMilliseconds += gpuDisplay_[focusedViewIndex][scopeIndex];
		}
		PROCESS_MEMORY_COUNTERS_EX memoryCounters{};
		memoryCounters.cb = sizeof(memoryCounters);
		Float ramMegabytes = GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memoryCounters), sizeof(memoryCounters)) ? static_cast<Float>(memoryCounters.WorkingSetSize) / (1024.0f * 1024.0f) : 0.0f;
		Float videoMemoryMegabytes = 0.0f;
		IDXGIAdapter4* adapter = context_.graphics_.graphics_->GetContext().GetAdapter()->Get();
		if (adapter)
		{
			DXGI_QUERY_VIDEO_MEMORY_INFO videoMemory{};
			if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemory)))
			{
				videoMemoryMegabytes = static_cast<Float>(videoMemory.CurrentUsage) / (1024.0f * 1024.0f);
			}
		}
		ImGui::Text("FPS %.1f | CPU %.2f ms | GPU %.2f ms | RAM %.0f MB | VRAM %.0f MB | DC %u", io.Framerate, frameMilliseconds, gpuMilliseconds, ramMegabytes, videoMemoryMegabytes, ProfilerStats::drawCallCount_);

		DrawFrameGraph(benchmark);

		ImGui::SeparatorText("フレーム予算（60 FPS）");

		/// [EN] FocusedView and GpuProfileView list Editor, Game and Canvas in the same order, so the focused view maps straight onto its GPU column.
		/// [JP] FocusedView と GpuProfileView は Editor、Game、Canvas を同じ順に並べているので、フォーカス中のビューはそのまま GPU の列に対応する。

		if (ImGui::BeginTable("##Budget", 2))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Bar", ImGuiTableColumnFlags_WidthStretch);

			/// [EN] One bar per measured total: the whole frame on the CPU side, and the focused view's passes on the GPU side.
			/// [JP] 計測した合計ごとに1本。CPU 側はフレーム全体、GPU 側はフォーカス中のビューのパス合計。
			const Char* labels[2] = { "フレーム", GpuProfiler::ViewName(static_cast<GpuProfileView>(focusedViewIndex)) };
			Float milliseconds[2] = { displayFrameMilliseconds_, gpuMilliseconds };

			for (Uint32 barIndex = 0; barIndex < 2; barIndex++)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextDisabled("%s", labels[barIndex]);
				ImGui::TableNextColumn();

				/// [EN] The bar is full at the budget; its color tells within budget, up to twice the budget (30 FPS), or beyond.
				/// [JP] バーは予算ちょうどで満タン。色で予算内、予算の2倍（30 FPS）まで、それ以上を見分ける。
				Float ratio = milliseconds[barIndex] / budgetMilliseconds_;
				ImVec4 barColor = ImVec4(0.35f, 0.8f, 0.45f, 1.0f);
				if (ratio > 2.0f)
				{
					barColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
				}
				else if (ratio > 1.0f)
				{
					barColor = ImVec4(1.0f, 0.8f, 0.35f, 1.0f);
				}

				Char overlay[64];
				snprintf(overlay, sizeof(overlay), "%.2f ms / %.2f ms", milliseconds[barIndex], budgetMilliseconds_);

				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
				ImGui::ProgressBar(Min(ratio, 1.0f), ImVec2(-FLT_MIN, 0.0f), overlay);
				ImGui::PopStyleColor();
			}

			ImGui::EndTable();
		}

		if (ImGui::BeginTabBar("##BenchmarkViews"))
		{
			if (ImGui::BeginTabItem("階層"))
			{
				DrawHierarchy(benchmark);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("タイムライン"))
			{
				DrawTimeline(benchmark);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("GPU"))
			{
				DrawGpuTable(gpuProfiler);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	/**
	* [EN]
	* Opens the stand-alone benchmark-profiler window.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 独立したベンチマークプロファイラーウィンドウを開く。
	*/
	void BenchmarkPanel::Open()
	{
		show_ = true;
	}

	/**
	* [EN]
	* Draws the most recent 300 frame times and selects one when clicked.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近 300 フレームの時間を描き、クリックしたフレームを選択する。
	*/
	void BenchmarkPanel::DrawFrameGraph(BenchmarkSystem& benchmark)
	{
		const BenchmarkFrames& frames = benchmark.Frames();
		if (frames.count_ == 0)
		{
			ImGui::TextDisabled("フレームを記録中です");
			return;
		}

		StaticArray<Float, BenchmarkFrames::capacity_> values{};
		Size count = Min(frames.count_, frames.capacity_);
		Size oldest = frames.count_ < frames.capacity_ ? 0 : frames.next_;
		for (Size index = 0; index < count; ++index)
		{
			values[index] = static_cast<Float>(frames.seconds_[(oldest + index) % frames.capacity_] * 1000.0);
		}

		ImGui::TextDisabled("フレームグラフ（クリックで選択・一時停止）");
		ImGui::PlotHistogram("##BenchmarkFrames", values.data(), static_cast<Int>(count), 0, nullptr, 0.0f, 50.0f, ImVec2(-FLT_MIN, 92.0f));
		ImVec2 graphMinimum = ImGui::GetItemRectMin();
		ImVec2 graphMaximum = ImGui::GetItemRectMax();
		ImDrawList* graphDrawList = ImGui::GetWindowDrawList();
		for (Float budgetMilliseconds : { 1000.0f / 60.0f, 1000.0f / 30.0f })
		{
			Float y = graphMaximum.y - (budgetMilliseconds / 50.0f) * (graphMaximum.y - graphMinimum.y);
			graphDrawList->AddLine(ImVec2(graphMinimum.x, y), ImVec2(graphMaximum.x, y), IM_COL32(240, 190, 80, 180));
		}
		if (ImGui::IsItemClicked())
		{
			Float relativeX = (ImGui::GetIO().MousePos.x - ImGui::GetItemRectMin().x) / ImGui::GetItemRectSize().x;
			Size frameIndex = Min(static_cast<Size>(Max(0.0f, relativeX) * count), count - 1);
			selectedSlot_ = static_cast<Int>((oldest + frameIndex) % frames.capacity_);
			benchmark.Paused(true);
		}
	}

	/**
	* [EN]
	* Draws the selected frame's call tree with total, self, call-count and history statistics.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 選択フレームの呼び出しツリーを、合計・自身・回数・履歴統計とともに描く。
	*/
	void BenchmarkPanel::DrawHierarchy(const BenchmarkSystem& benchmark)
	{
		const BenchmarkFrames& frames = benchmark.Frames();
		if (frames.count_ == 0)
		{
			return;
		}

		Size slot = selectedSlot_ >= 0 ? static_cast<Size>(selectedSlot_) : (frames.next_ + frames.capacity_ - 1) % frames.capacity_;
		Double frameSeconds = frames.seconds_[slot];
		std::span<const BenchmarkNode> nodes = benchmark.Nodes();
		Float tableHeight = Max(320.0f, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y);
		if (!ImGui::BeginTable("##BenchmarkHierarchy", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImVec2(0.0f, tableHeight)))
		{
			return;
		}

		ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("合計", ImGuiTableColumnFlags_WidthFixed, 72.0f);
		ImGui::TableSetupColumn("自身", ImGuiTableColumnFlags_WidthFixed, 72.0f);
		ImGui::TableSetupColumn("回数", ImGuiTableColumnFlags_WidthFixed, 48.0f);
		ImGui::TableSetupColumn("フレーム比", ImGuiTableColumnFlags_WidthFixed, 110.0f);
		ImGui::TableSetupColumn("平均", ImGuiTableColumnFlags_WidthFixed, 72.0f);
		ImGui::TableSetupColumn("最大", ImGuiTableColumnFlags_WidthFixed, 72.0f);
		ImGui::TableHeadersRow();

		for (const BenchmarkNode& node : nodes)
		{
			if (search_[0] && std::string_view(node.name_).find(search_) == std::string_view::npos)
			{
				continue;
			}

			Double selfSeconds = node.seconds_[slot];
			for (Size child : node.children_)
			{
				selfSeconds -= nodes[child].seconds_[slot];
			}
			Double totalSeconds = 0.0;
			Double maximumSeconds = 0.0;
			Size sampleCount = Min(frames.count_, frames.capacity_);
			for (Size sample = 0; sample < sampleCount; ++sample)
			{
				totalSeconds += node.seconds_[sample];
				maximumSeconds = Max(maximumSeconds, node.seconds_[sample]);
			}

			ImGui::TableNextRow(ImGuiTableRowFlags_None, ImGui::GetTextLineHeightWithSpacing() + 3.0f);
			ImGui::TableNextColumn();
			ImGui::Indent(static_cast<Float>(node.depth_) * 14.0f);
			ImGui::TextUnformatted(node.name_.c_str());
			ImGui::Unindent(static_cast<Float>(node.depth_) * 14.0f);
			if (ImGui::IsItemHovered())
			{
				StaticArray<Float, BenchmarkFrames::capacity_> history{};
				for (Size sample = 0; sample < sampleCount; ++sample)
				{
					history[sample] = static_cast<Float>(node.seconds_[(frames.next_ + frames.capacity_ - sampleCount + sample) % frames.capacity_] * 1000.0);
				}
				ImGui::BeginTooltip();
				ImGui::PlotLines("##NodeHistory", history.data(), static_cast<Int>(sampleCount), 0, nullptr, 0.0f, FLT_MAX, ImVec2(280.0f, 70.0f));
				ImGui::EndTooltip();
			}
			ImGui::TableNextColumn(); ImGui::Text("%.2f ms", node.seconds_[slot] * 1000.0);
			ImGui::TableNextColumn(); ImGui::Text("%.2f ms", Max(0.0, selfSeconds) * 1000.0);
			ImGui::TableNextColumn(); ImGui::Text("%u", node.calls_[slot]);
			ImGui::TableNextColumn(); ImGui::ProgressBar(frameSeconds > 0.0 ? Min(static_cast<Float>(node.seconds_[slot] / frameSeconds), 1.0f) : 0.0f, ImVec2(-FLT_MIN, 0.0f));
			ImGui::TableNextColumn(); ImGui::Text("%.2f ms", totalSeconds / sampleCount * 1000.0);
			ImGui::TableNextColumn(); ImGui::Text("%.2f ms", maximumSeconds * 1000.0);
		}
		ImGui::EndTable();
	}

	/**
	* [EN]
	* Draws the latest captured events as nested bars for each attached thread.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Attach 済みのスレッドごとに、最後に取り込んだイベントを入れ子のバーで描く。
	*/
	void BenchmarkPanel::DrawTimeline(const BenchmarkSystem& benchmark)
	{
		const BenchmarkCapture& capture = benchmark.Capture();
		Double ticks = Max(1.0, static_cast<Double>(capture.endTicks_ - capture.beginTicks_));
		ImGui::TextDisabled("ホイールで拡大、ドラッグで移動。表示中の生イベントは最新または一時停止フレームです。");
		Float timelineHeight = Max(320.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing());
		ImGui::BeginChild("##BenchmarkTimeline", ImVec2(0.0f, timelineHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		if (ImGui::IsWindowHovered())
		{
			Float wheel = ImGui::GetIO().MouseWheel;
			if (wheel != 0.0f)
			{
				timelineScale_ = Min(Max(timelineScale_ * (wheel > 0.0f ? 1.15f : 1.0f / 1.15f), 0.25f), 12.0f);
			}
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				timelineOffset_ += ImGui::GetIO().MouseDelta.x;
			}
		}

		Float width = 900.0f * timelineScale_;
		for (const BenchmarkLane& lane : capture.lanes_)
		{
			ImGui::TextUnformatted(lane.name_.c_str());
			ImGui::SameLine(130.0f);
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(origin, ImVec2(width + origin.x, 48.0f + origin.y), IM_COL32(42, 42, 48, 255));
			for (const BenchmarkEvent& event : lane.events_)
			{
				Uint64 eventStart = event.startTicks_ > capture.beginTicks_ ? event.startTicks_ - capture.beginTicks_ : 0;
				Float left = timelineOffset_ + static_cast<Float>(eventStart / ticks) * width;
				Float eventWidth = Max(1.0f, static_cast<Float>(event.ticks_ / ticks) * width);
				Float top = origin.y + static_cast<Float>(event.depth_ % 5) * 8.0f;
				ImU32 color = IM_COL32(72 + (event.depth_ * 37) % 128, 152, 232, 230);
				ImVec2 eventMinimum = ImVec2(origin.x + left, top);
				ImVec2 eventMaximum = ImVec2(origin.x + Min(left + eventWidth, width), top + 7.0f);
				drawList->AddRectFilled(eventMinimum, eventMaximum, color);
				if (ImGui::IsMouseHoveringRect(eventMinimum, eventMaximum))
				{
					ImGui::BeginTooltip();
					ImGui::Text("%s", event.name_);
					ImGui::TextDisabled("%.3f ms | depth %u", event.ticks_ * capture.secondsPerTick_ * 1000.0, event.depth_);
					ImGui::EndTooltip();
				}
			}
			ImGui::Dummy(ImVec2(width, 50.0f));
		}
		ImGui::EndChild();
	}

	/**
	* [EN]
	* Draws the CPU sections grouped by their "Group/" prefix. Sections of one
	* group do not overlap each other, so the group row shows their sum; each
	* section row shows its share of the frame as a bar, and hovering it plots
	* the section's whole history.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CPU 区間を "グループ/" の接頭辞ごとにまとめて描く。同じグループの区間は
	* 互いに重ならないので、グループの行にはその合計を出す。各区間の行には
	* フレームに占める割合をバーで出し、ホバーすると区間の履歴全体を描く。
	*/
	void BenchmarkPanel::DrawCpuTable(const BenchmarkSystem& benchmark)
	{
		ImGui::SeparatorText("CPU（区間別）");

		if (rows_.empty())
		{
			ImGui::TextDisabled("計測された区間がありません");
			return;
		}

		if (ImGui::BeginTable("##CpuStats", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("区間", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("平均", ImGuiTableColumnFlags_WidthFixed, 78.0f);
			ImGui::TableSetupColumn("最大", ImGuiTableColumnFlags_WidthFixed, 78.0f);
			ImGui::TableSetupColumn("回数", ImGuiTableColumnFlags_WidthFixed, 56.0f);
			ImGui::TableSetupColumn("フレーム比", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableHeadersRow();

			/// [EN] rows_ is sorted by group, so each group is one contiguous run [rowIndex, groupEnd).
			/// [JP] rows_ はグループ順に並んでいるので、各グループは連続した範囲 [rowIndex, groupEnd) になる。
			Size rowIndex = 0;
			while (rowIndex < rows_.size())
			{
				Size groupEnd = rowIndex;
				Double groupMilliseconds = 0.0;
				while (groupEnd < rows_.size() && rows_[groupEnd].groupRank_ == rows_[rowIndex].groupRank_)
				{
					groupMilliseconds += rows_[groupEnd].averageMilliseconds_;
					groupEnd++;
				}

				/// [EN] Group row: a collapsible header carrying the group's total and its share of the frame.
				/// [JP] グループの行。グループの合計とフレームに占める割合を持つ、折りたためる見出し。
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				Bool open = ImGui::TreeNodeEx(rows_[rowIndex].group_.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f ms", groupMilliseconds);
				ImGui::TableNextColumn();
				ImGui::TableNextColumn();
				ImGui::TableNextColumn();
				Float groupRatio = displayFrameMilliseconds_ > 0.0f ? static_cast<Float>(groupMilliseconds) / displayFrameMilliseconds_ : 0.0f;
				Char groupOverlay[32];
				snprintf(groupOverlay, sizeof(groupOverlay), "%.1f%%", groupRatio * 100.0f);
				ImGui::ProgressBar(Min(groupRatio, 1.0f), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight()), groupOverlay);

				if (open)
				{
					for (Size index = rowIndex; index < groupEnd; index++)
					{
						const BenchmarkRow& row = rows_[index];

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TreeNodeEx(row.label_.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);

						/// [EN] Hovering a section plots every frame still in its ring, oldest on the left. Histories only grow, so the latched index stays valid.
						/// [JP] 区間にホバーすると、リングに残る全フレームを古い順に左から描く。履歴は増えるだけなので、写し取った添字は有効なまま。
						if (ImGui::IsItemHovered())
						{
							const BenchmarkHistory& history = benchmark.Histories()[row.historyIndex_];
							/// [EN] The benchmark ring holds 300 frames, so the tooltip plot must hold the same maximum.
							/// [JP] ベンチマークのリングは 300 フレームなので、ツールチップの折れ線も同じ最大数を持つ。
							StaticArray<Float, BenchmarkFrames::capacity_> values{};
							Size ringSize = history.seconds_.size();
							Size valueCount = Min(history.count_, ringSize);
							Size oldest = history.count_ < ringSize ? 0 : history.next_;
							for (Size valueIndex = 0; valueIndex < valueCount; valueIndex++)
							{
								values[valueIndex] = static_cast<Float>(history.seconds_[(oldest + valueIndex) % ringSize] * 1000.0);
							}

							Char plotOverlay[64];
							snprintf(plotOverlay, sizeof(plotOverlay), "平均 %.2f ms / 最大 %.2f ms", row.averageMilliseconds_, row.maximumMilliseconds_);

							ImGui::BeginTooltip();
							ImGui::TextUnformatted(history.name_.c_str());
							ImGui::PlotLines("##History", values.data(), static_cast<Int>(valueCount), 0, plotOverlay, 0.0f, FLT_MAX, ImVec2(320.0f, 80.0f));
							ImGui::EndTooltip();
						}

						/// [EN] A section that did not run in the averaged frames shows "-", so "not running" is not mistaken for "fast".
						/// [JP] 平均を取ったフレームで走らなかった区間は「-」にし、「走っていない」を「速い」と取り違えないようにする。
						if (row.averageCalls_ <= 0.0)
						{
							for (Uint32 columnIndex = 0; columnIndex < 4; columnIndex++)
							{
								ImGui::TableNextColumn();
								ImGui::TextDisabled("-");
							}
							continue;
						}

						/// [EN] Heavy sections are colored so they stand out (1 ms and 4 ms thresholds, same as the GPU table).
						/// [JP] 重い区間を目で拾えるように色を付ける（GPU の表と同じ 1ms/4ms 基準）。
						ImGui::TableNextColumn();
						ImVec4 averageColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
						if (row.averageMilliseconds_ >= 4.0)
						{
							averageColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
						}
						else if (row.averageMilliseconds_ >= 1.0)
						{
							averageColor = ImVec4(1.0f, 0.8f, 0.35f, 1.0f);
						}
						ImGui::TextColored(averageColor, "%.2f ms", row.averageMilliseconds_);

						ImGui::TableNextColumn();
						ImGui::TextDisabled("%.2f ms", row.maximumMilliseconds_);

						/// [EN] Call count reveals per-actor sections: a count that grows with the actor count means the cost scales with it.
						/// [JP] 回数は Actor ごとの区間を見分ける手がかり。Actor 数とともに増えるなら、コストもそれに比例する。
						ImGui::TableNextColumn();
						ImGui::Text("%.0f", row.averageCalls_);

						ImGui::TableNextColumn();
						Float ratio = displayFrameMilliseconds_ > 0.0f ? static_cast<Float>(row.averageMilliseconds_) / displayFrameMilliseconds_ : 0.0f;
						Char overlay[32];
						snprintf(overlay, sizeof(overlay), "%.1f%%", ratio * 100.0f);
						ImGui::ProgressBar(Min(ratio, 1.0f), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight()), overlay);
					}

					ImGui::TreePop();
				}

				rowIndex = groupEnd;
			}

			ImGui::EndTable();
		}

		ImGui::TextDisabled("※平均・最大は直近 %zu フレーム。ホバーで履歴を表示", averageFrameCount_);
	}

	/**
	* [EN]
	* Per-pass GPU times, one column per view. The editor and the game view run
	* the same pass list against different cameras, so seeing them side by side
	* attributes a frame-rate drop to a specific pass. The focused view's column
	* is highlighted, since it is the one the budget bar above sums up.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パス別 GPU 時間。ビューごとに列を分ける。エディタとゲームビューは同じパス列を
	* 別カメラで回すので、両者を並べて見ることで FPS の低下を特定のパスに帰着できる。
	* 上の予算バーが合計しているのはフォーカス中のビューなので、その列を強調する。
	*/
	void BenchmarkPanel::DrawGpuTable(const GpuProfiler& gpuProfiler)
	{
		ImGui::SeparatorText("GPU（パス別）");

		if (!gpuProfiler.Available())
		{
			ImGui::TextDisabled("GPU タイムスタンプが利用できません");
			return;
		}

		const Uint32 viewCount = static_cast<Uint32>(GpuProfileView::Count);
		const Uint32 scopeCount = static_cast<Uint32>(GpuProfileScope::Count);
		Uint32 focusedViewIndex = static_cast<Uint32>(context_.view_.focused_);

		/// [EN] Translucent tint laid over the focused view's cells.
		/// [JP] フォーカス中のビューのセルに重ねる半透明の色。
		ImU32 focusedColor = ImGui::GetColorU32(ImVec4(0.35f, 0.55f, 1.0f, 0.15f));

		if (ImGui::BeginTable("##GpuStats", 1 + static_cast<Int>(viewCount), ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("パス", ImGuiTableColumnFlags_WidthStretch);
			for (Uint32 viewIndex = 0; viewIndex < viewCount; viewIndex++)
			{
				ImGui::TableSetupColumn(GpuProfiler::ViewName(static_cast<GpuProfileView>(viewIndex)), ImGuiTableColumnFlags_WidthFixed, 78.0f);
			}
			ImGui::TableHeadersRow();

			Float totals[static_cast<Uint32>(GpuProfileView::Count)] = {};

			for (Uint32 scopeIndex = 0; scopeIndex < scopeCount; scopeIndex++)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextDisabled("%s", GpuProfiler::ScopeName(static_cast<GpuProfileScope>(scopeIndex)));

				for (Uint32 viewIndex = 0; viewIndex < viewCount; viewIndex++)
				{
					ImGui::TableNextColumn();
					if (viewIndex == focusedViewIndex)
					{
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, focusedColor);
					}

					Float milliseconds = gpuDisplay_[viewIndex][scopeIndex];
					totals[viewIndex] += milliseconds;

					/// [EN] A pass that did not run reads 0; "-" keeps "disabled" from looking like "fast".
					/// [JP] 走っていないパスは 0 が返るので、0.0 と表示するより「-」の方が「無効」と「速い」を混同しない。
					if (milliseconds <= 0.0f)
					{
						ImGui::TextDisabled("-");
						continue;
					}

					/// [EN] Heavy passes are colored so they stand out (1 ms and 4 ms thresholds).
					/// [JP] 重いパスを目で拾えるように色を付ける（1ms/4ms 基準）。
					ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
					if (milliseconds >= 4.0f)
					{
						color = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
					}
					else if (milliseconds >= 1.0f)
					{
						color = ImVec4(1.0f, 0.8f, 0.35f, 1.0f);
					}
					ImGui::TextColored(color, "%.2f ms", milliseconds);
				}
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("合計");
			for (Uint32 viewIndex = 0; viewIndex < viewCount; viewIndex++)
			{
				ImGui::TableNextColumn();
				if (viewIndex == focusedViewIndex)
				{
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, focusedColor);
				}
				ImGui::Text("%.2f ms", totals[viewIndex]);
			}

			ImGui::EndTable();
		}

		ImGui::TextDisabled("※計測対象パスの合計です（フレーム全体ではありません）");
	}
}
