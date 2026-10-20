#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	BenchmarkSystem::BenchmarkSystem()
	{
		Int registers[4]{};
		__cpuid(registers, static_cast<Int>(0x80000007));
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
		secondsPerTick_ = elapsedSeconds / static_cast<Double>(ticksEnd - ticksStart);
	}

	BenchmarkSystem::~BenchmarkSystem()
	{
		Detach();
	}

	void BenchmarkSystem::Attach()
	{
		std::scoped_lock lock(mutex_);
		threads_.push_back(MakePtr<BenchmarkThread>());
		Benchmark::Bind(threads_.back().get());
	}

	void BenchmarkSystem::Detach()
	{
		Benchmark::Bind(nullptr);
	}

	void BenchmarkSystem::Collect()
	{
		std::scoped_lock lock(mutex_);

		for (const ResourcePtr<BenchmarkThread>& thread : threads_)
		{
			for (Size chunkIndex = 0;chunkIndex <= thread->chunkIndex_;++chunkIndex)
			{
				const BenchmarkEvent* begin = thread->chunks_[chunkIndex].data();
				const BenchmarkEvent* end = chunkIndex == thread->chunkIndex_ ? thread->write_ : begin + thread->chunks_[chunkIndex].size();

				for (const BenchmarkEvent* event = begin;event != end;++event)
				{
					Size historyIndex = 0;
					auto indexIt = nameIndex_.find(event->name_);
					if (indexIt != nameIndex_.end())
					{
						historyIndex = indexIt->second;
					}
					else
					{
						String name = String(std::string_view(event->name_));
						auto historyIt = std::ranges::find(histories_, name, &BenchmarkHistory::name_);
						if (historyIt == histories_.end())
						{
							histories_.push_back(BenchmarkHistory{ name });
							historyIndex = histories_.size() - 1;
						}
						else
						{
							historyIndex = static_cast<Size>(historyIt - histories_.begin());
						}
						nameIndex_[event->name_] = historyIndex;
					}

					BenchmarkHistory& history = histories_[historyIndex];
					history.seconds_[history.next_] = static_cast<Double>(event->ticks_) * secondsPerTick_;
					history.next_ = (history.next_ + 1) % history.seconds_.size();
					++history.count_;
				}
			}
			thread->Rewind();

			if (thread->mismatchCount_ > 0 || thread->unmatchedEndCount_ > 0)
			{
				SC_LOG_WARNING("ベンチマーク: Begin と End の名前が合わないものが {} 件、対応する Begin が無い End が {} 件ありました。", thread->mismatchCount_, thread->unmatchedEndCount_);
				thread->mismatchCount_ = 0;
				thread->unmatchedEndCount_ = 0;
			}
		}
	}

	std::span<const BenchmarkHistory> BenchmarkSystem::Histories()const
	{
		return histories_;
	}
}