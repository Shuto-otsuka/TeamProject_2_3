#pragma once

namespace SeedCore
{
	struct BenchmarkOpenSection
	{
		const Char* name_ = nullptr;
		Uint64 startTicks_ = 0;
	};

	struct BenchmarkEvent
	{
		const Char* name_ = nullptr;
		Uint64 ticks_ = 0;
	};

	class SEEDCORE_API BenchmarkThread :public NonTransferable
	{
	private:
		friend class Benchmark;
		friend class BenchmarkSystem;

	public:
		BenchmarkThread();

	private:
		__declspec(noinline) void Grow();

		void Rewind();

	private:
		alignas(64) Size openCount_ = 0;

		BenchmarkEvent* write_ = nullptr;

		BenchmarkEvent* end_ = nullptr;

		Size chunkIndex_ = 0;

		Size overflowDepth_ = 0;

		Size mismatchCount_ = 0;

		Size unmatchedEndCount_ = 0;

		StaticArray<BenchmarkOpenSection, 64> open_{};

		DynamicArray<DynamicArray<BenchmarkEvent>> chunks_;
	};
}