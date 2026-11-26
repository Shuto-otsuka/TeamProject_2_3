#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	BenchmarkSystem::BenchmarkSystem()
	{
		Int registers[4]{};
		__cpuidex(registers, static_cast<Int>(0x80000007), 0);
		if ((registers[3] & (1 << 8)) == 0)
		{
			SC_LOG_WARNING("ベンチマーク: この CPU のカウンタはコアごとに進み方が違う可能性があります。計測値が不正確になることがあります。");
		}

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

		lastCollectTicks_ = ticksEnd;
	}

	BenchmarkSystem::~BenchmarkSystem()
	{
		Detach();
	}

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

	void BenchmarkSystem::Detach()
	{
		Benchmark::Bind(nullptr);
	}

	void BenchmarkSystem::Collect()
	{
		std::scoped_lock lock(mutex_);

		Uint64 nowTicks = __rdtsc();
		Size slot = frames_.next_;

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

			while (thread.lock_.test_and_set(std::memory_order_acquire))
			{
				_mm_pause();
			}

			if (!paused_)
			{
				events.clear();

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

			std::ranges::sort(events, [](const BenchmarkEvent& left, const BenchmarkEvent& right)
				{
					if (left.startTicks_ != right.startTicks_)
					{
						return left.startTicks_ < right.startTicks_;
					}
					return left.depth_ < right.depth_;
				});

			Size root = roots_[threadIndex];
			stack_.clear();
			for (const BenchmarkEvent& event : events)
			{
				Size depth = Min(static_cast<Size>(event.depth_), stack_.size());
				stack_.resize(depth);
				Size parent = depth == 0 ? root : stack_[depth - 1];

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

			for (BenchmarkNode& node : nodes_)
			{
				node.count_ = frames_.count_;
				node.next_ = frames_.next_;
			}
		}

		lastCollectTicks_ = nowTicks;
	}

	std::span<const BenchmarkNode> BenchmarkSystem::Nodes()const
	{
		return nodes_;
	}

	std::span<const BenchmarkHistory> BenchmarkSystem::Histories()const
	{
		return nodes_;
	}

	const BenchmarkFrames& BenchmarkSystem::Frames()const
	{
		return frames_;
	}

	const BenchmarkCapture& BenchmarkSystem::Capture()const
	{
		return capture_;
	}

	Bool BenchmarkSystem::Paused()const
	{
		return paused_;
	}

	void BenchmarkSystem::Paused(Bool paused)
	{
		paused_ = paused;
	}
}
