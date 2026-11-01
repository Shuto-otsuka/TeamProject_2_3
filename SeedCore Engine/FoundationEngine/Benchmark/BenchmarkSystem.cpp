#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	/**
	* [EN]
	* Measures how many seconds one CPU timestamp tick lasts.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CPU のタイムスタンプ 1 tick が何秒にあたるかを測る。
	*/
	BenchmarkSystem::BenchmarkSystem()
	{
		/// [EN] CPUID 0x80000007 EDX bit 8 is the invariant TSC flag: without it the counter rate may differ per core or change with the clock.
		/// [JP] CPUID 0x80000007 の EDX ビット8は不変 TSC のフラグ。これが無いとカウンタの進み方がコアやクロックで変わりうる。
		Int registers[4]{};
		__cpuidex(registers, static_cast<Int>(0x80000007), 0);
		if ((registers[3] & (1 << 8)) == 0)
		{
			SC_LOG_WARNING("ベンチマーク: この CPU のカウンタはコアごとに進み方が違う可能性があります。計測値が不正確になることがあります。");
		}

		/// [EN] The tick length is calibrated against the performance counter, whose frequency is known, over a short sleep.
		/// [JP] tick の長さは、周波数が分かっているパフォーマンスカウンタと短い Sleep の間で突き合わせて求める。
		LARGE_INTEGER frequency{};
		LARGE_INTEGER counterStart{};
		LARGE_INTEGER counterEnd{};
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&counterStart);
		Uint64 ticksStart = __rdtsc();
		Sleep(50);
		QueryPerformanceCounter(&counterEnd);
		Uint64 ticksEnd = __rdtsc();

		Double elapsedSeconds = static_cast<Double>(counterEnd.QuadPart - counterStart.QuadPart) / static_cast<Double>(frequency.QuadPart);
		capture_.secondsPerTick_ = elapsedSeconds / static_cast<Double>(ticksEnd - ticksStart);

		/// [EN] The first frame is measured from here.
		/// [JP] 最初のフレームはここから測る。
		lastCollectTicks_ = ticksEnd;
	}

	/**
	* [EN]
	* Unbinds the calling thread from its event buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼んだスレッドをイベントバッファから外す。
	*/
	BenchmarkSystem::~BenchmarkSystem()
	{
		Detach();
	}

	/**
	* [EN]
	* Gives the calling thread its own event buffer and call-tree root
	* under the given name, so its Benchmark::Begin/End calls start being
	* recorded. Safe to call from any thread.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼んだスレッドに、指定の名前で専用のイベントバッファと呼び出しツリーの
	* 根を持たせ、そのスレッドの Benchmark::Begin/End が記録されるように
	* する。どのスレッドから呼んでもよい。
	*/
	void BenchmarkSystem::Attach(String name)
	{
		std::scoped_lock lock(mutex_);

		threads_.push_back(MakePtr<BenchmarkThread>(name));

		BenchmarkNode root;
		root.name_ = name;
		nodes_.push_back(std::move(root));
		roots_.push_back(nodes_.size() - 1);

		capture_.lanes_.push_back(BenchmarkLane{ name });

		Benchmark::Bind(threads_.back().get());
	}

	/**
	* [EN]
	* Unbinds the calling thread, so its Benchmark::Begin/End calls are ignored.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼んだスレッドを外し、そのスレッドの Benchmark::Begin/End を無視させる。
	*/
	void BenchmarkSystem::Detach()
	{
		Benchmark::Bind(nullptr);
	}

	/**
	* [EN]
	* Ends the frame: drains every thread's events, and unless paused,
	* folds them into the node rings and keeps them as the capture. Call
	* once per frame from the main thread, outside every section.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームを締める。全スレッドのイベントを吸い出し、一時停止中でなければ
	* ノードのリングへまとめ、取り込みとして残す。メインスレッドから、どの区間の
	* 外側でもフレームに1回呼ぶ。
	*/
	void BenchmarkSystem::Collect()
	{
		std::scoped_lock lock(mutex_);

		Uint64 nowTicks = __rdtsc();
		Size slot = frames_.next_;

		/// [EN] The slot this frame writes into is cleared first, so a node that did not run this frame records zero.
		/// [JP] このフレームが書き込む枠を先に空にし、このフレームで走らなかったノードには 0 が残るようにする。
		if (!paused_)
		{
			for (BenchmarkNode& node : nodes_)
			{
				node.seconds_[slot] = 0.0;
				node.calls_[slot] = 0;
			}
			capture_.beginTicks_ = lastCollectTicks_;
			capture_.endTicks_ = nowTicks;
		}

		for (Size threadIndex = 0; threadIndex < threads_.size(); threadIndex++)
		{
			BenchmarkThread& thread = *threads_[threadIndex];
			DynamicArray<BenchmarkEvent>& events = capture_.lanes_[threadIndex].events_;

			/// [EN] The events are copied out under the thread's lock and processed after it is released, so the thread waits only for the copy.
			/// [JP] イベントはスレッドのロックを持ったまま写し出し、処理はロックを放してから行う。スレッドが待つのはコピーの間だけ。
			while (thread.lock_.test_and_set(std::memory_order_acquire))
			{
				_mm_pause();
			}

			if (!paused_)
			{
				events.clear();

				/// [EN] Events live in a chain of fixed-size chunks; only the current chunk is partly filled, up to write_.
				/// [JP] イベントは固定長チャンクの連なりに入っている。途中までしか埋まっていないのは今のチャンクだけで、write_ までが有効。
				for (Size chunkIndex = 0; chunkIndex <= thread.chunkIndex_; chunkIndex++)
				{
					const BenchmarkEvent* begin = thread.chunks_[chunkIndex].data();
					const BenchmarkEvent* end = chunkIndex == thread.chunkIndex_ ? thread.write_ : begin + thread.chunks_[chunkIndex].size();
					events.insert(events.end(), begin, end);
				}
			}
			thread.Rewind();

			thread.lock_.clear(std::memory_order_release);

			if (thread.mismatchCount_ > 0 || thread.unmatchedEndCount_ > 0)
			{
				SC_LOG_WARNING("ベンチマーク: Begin と End の名前が合わないものが {} 件、対応する Begin が無い End が {} 件ありました。", thread.mismatchCount_, thread.unmatchedEndCount_);
				thread.mismatchCount_ = 0;
				thread.unmatchedEndCount_ = 0;
			}

			if (paused_)
			{
				continue;
			}

			/// [EN] Events are recorded when they end, so children come before their parent; sorting by start (outer first on a tie) puts every parent before its children.
			/// [JP] イベントは終わった時点で記録されるので子が親より先に並ぶ。開始順（同時なら外側が先）に並べ替えると、親が必ず子より前に来る。
			std::ranges::sort(events, [](const BenchmarkEvent& left, const BenchmarkEvent& right)
				{
					if (left.startTicks_ != right.startTicks_)
					{
						return left.startTicks_ < right.startTicks_;
					}
					return left.depth_ < right.depth_;
				});

			/// [EN] Walking in start order, stack_[depth] is the node of the innermost open section at that depth, which is the parent of the next deeper event.
			/// [JP] 開始順にたどると、stack_[depth] はその深さで開いている一番内側の区間のノードで、次に来る一段深いイベントの親になる。
			Size root = roots_[threadIndex];
			stack_.clear();
			for (const BenchmarkEvent& event : events)
			{
				/// [EN] A section whose parent began in an earlier frame has no parent here, so its depth is clamped onto the open stack.
				/// [JP] 親が前のフレームで始まった区間はここに親が無いので、深さを開いているスタックの範囲に収める。
				Size depth = Min(static_cast<Size>(event.depth_), stack_.size());
				stack_.resize(depth);
				Size parent = depth == 0 ? root : stack_[depth - 1];

				/// [EN] The literal's address finds the child directly; a different address falls back to comparing names, since the same name can come from literals at different addresses.
				/// [JP] リテラルのアドレスで子を直接探す。アドレスが違えば名前で比べ直す。同じ名前でも別アドレスのリテラルから来ることがあるため。
				Size node = nodes_.size();
				for (Size child : nodes_[parent].children_)
				{
					if (nodes_[child].key_ == event.name_ || static_cast<std::string_view>(nodes_[child].name_) == std::string_view(event.name_))
					{
						node = child;
						break;
					}
				}

				if (node == nodes_.size())
				{
					BenchmarkNode created;
					created.name_ = String(std::string_view(event.name_));
					created.key_ = event.name_;
					created.depth_ = nodes_[parent].depth_ + 1;
					nodes_.push_back(std::move(created));
					nodes_[parent].children_.push_back(node);
				}

				Double seconds = static_cast<Double>(event.ticks_) * capture_.secondsPerTick_;
				nodes_[node].seconds_[slot] += seconds;
				nodes_[node].calls_[slot]++;

				/// [EN] The thread root accumulates only top-level sections, which do not overlap, so it holds the thread's busy time.
				/// [JP] スレッドの根には重ならない最上位の区間だけを足すので、スレッドが働いていた時間になる。
				if (depth == 0)
				{
					nodes_[root].seconds_[slot] += seconds;
					nodes_[root].calls_[slot]++;
				}

				stack_.push_back(node);
			}
		}

		if (!paused_)
		{
			frames_.seconds_[slot] = static_cast<Double>(nowTicks - lastCollectTicks_) * capture_.secondsPerTick_;
			frames_.next_ = (frames_.next_ + 1) % frames_.capacity_;
			++frames_.count_;

			/// [EN] The compatibility view shares every node's rings with BenchmarkFrames.
			/// [JP] 互換ビューは BenchmarkFrames と各ノードのリングを共有する。
			for (BenchmarkNode& node : nodes_)
			{
				node.count_ = frames_.count_;
				node.next_ = frames_.next_;
			}
		}

		lastCollectTicks_ = nowTicks;
	}

	/**
	* [EN]
	* Every node of the call tree; depth-0 nodes are the thread roots.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼び出しツリーの全ノード。深さ 0 のノードがスレッドの根。
	*/
	std::span<const BenchmarkNode> BenchmarkSystem::Nodes()const
	{
		return nodes_;
	}

	/**
	* [EN]
	* Returns the call-tree nodes through the former flat-history interface.
	* This lets existing clients migrate their presentation incrementally.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 以前のフラット履歴インターフェースとして呼び出しツリーノードを返す。
	* 既存の表示は段階的に移行できる。
	*/
	std::span<const BenchmarkHistory> BenchmarkSystem::Histories()const
	{
		return nodes_;
	}

	/**
	* [EN]
	* Per-frame times, whose slot index the node rings share.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームごとの時間。ノードのリングはこの添字を共有する。
	*/
	const BenchmarkFrames& BenchmarkSystem::Frames()const
	{
		return frames_;
	}

	/**
	* [EN]
	* Raw events of the latest collected frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最後に集計したフレームの生イベント。
	*/
	const BenchmarkCapture& BenchmarkSystem::Capture()const
	{
		return capture_;
	}

	/**
	* [EN]
	* Whether recording is paused.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 記録を一時停止しているかどうか。
	*/
	Bool BenchmarkSystem::Paused()const
	{
		return paused_;
	}

	/**
	* [EN]
	* Pauses or resumes recording. While paused, events are still drained
	* so the thread buffers do not grow, but the rings and the capture are
	* left untouched, so the frames on screen can be inspected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 記録を一時停止・再開する。停止中もスレッドのバッファが膨らまないよう
	* イベントは吸い出すが、リングと取り込みには触れず、画面上のフレームを
	* じっくり見られるようにする。
	*/
	void BenchmarkSystem::Paused(Bool paused)
	{
		paused_ = paused;
	}
}
