#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Benchmark/BenchmarkThread.h>

namespace SeedCore
{
	namespace
	{
		constinit thread_local BenchmarkThread* currentThread = nullptr;
	}

	void Benchmark::BeginLiteral(const Char* name)
	{
		BenchmarkThread* thread = currentThread;
		if (!thread) [[unlikely]]
		{
			return;
		}

		if (thread->openCount_ == thread->open_.size()) [[unlikely]]
		{
			++thread->overflowDepth_;
			return;
		}

		std::atomic_signal_fence(std::memory_order_seq_cst);
		thread->open_[thread->openCount_++] = BenchmarkOpenSection{ name,__rdtsc() };
		std::atomic_signal_fence(std::memory_order_seq_cst);
	}

	void Benchmark::EndLiteral(const Char* name)
	{
		std::atomic_signal_fence(std::memory_order_seq_cst);
		Uint32 processor = 0;
		Uint64 endTicks = __rdtscp(&processor);
		std::atomic_signal_fence(std::memory_order_seq_cst);

		BenchmarkThread* thread = currentThread;

		if (!thread) [[unlikely]]
		{
			return;
		}

		if (thread->overflowDepth_ > 0) [[unlikely]]
		{
			--thread->overflowDepth_;
			return;
		}

		if (thread->openCount_ == 0) [[unlikely]]
		{
			++thread->unmatchedEndCount_;
			return;
		}

		const BenchmarkOpenSection& open = thread->open_[--thread->openCount_];
		if (open.name_ != name && std::strcmp(open.name_, name) != 0) [[unlikely]]
		{
			++thread->mismatchCount_;
			return;
		}

		if (thread->write_ == thread->end_) [[unlikely]]
		{
			thread->Grow();
		}
		*thread->write_++ = BenchmarkEvent{ name,endTicks - open.startTicks_ };
	}

	void Benchmark::Bind(BenchmarkThread* thread)
	{
		currentThread = thread;
	}
}