#pragma once
#include <FoundationEngine/Benchmark/BenchmarkThread.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	struct BenchmarkHistory
	{
		String name_;

		StaticArray<Double, 256> seconds_{};

		Size count_ = 0;

		Size next_ = 0;
	};

	class SEEDCORE_API BenchmarkSystem :public NonTransferable
	{
	public:
		BenchmarkSystem();

		~BenchmarkSystem();

		void Attach();

		void Detach();

		void Collect();

		std::span<const BenchmarkHistory> Histories()const;

	private:
		DynamicArray<ResourcePtr<BenchmarkThread>> threads_;

		std::mutex mutex_;

		Double secondsPerTick_ = 0.0;

		FlatMap<const Char*, Size> nameIndex_;

		DynamicArray<BenchmarkHistory> histories_;
	};
}