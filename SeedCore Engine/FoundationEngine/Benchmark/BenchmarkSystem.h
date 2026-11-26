#pragma once
#include <FoundationEngine/Benchmark/BenchmarkThread.h>

namespace SeedCore
{
	struct BenchmarkFrames
	{
		SC_CONSTEX Size capacity_ = 300;

		StaticArray<Double, capacity_> seconds_{};

		Size count_ = 0;

		Size next_ = 0;
	};

	struct BenchmarkNode
	{
		String name_;

		const Char* key_ = nullptr;

		Uint32 depth_ = 0;

		DynamicArray<Size> children_;

		StaticArray<Double, BenchmarkFrames::capacity_> seconds_{};

		StaticArray<Uint32, BenchmarkFrames::capacity_> calls_{};

		Size count_ = 0;

		Size next_ = 0;
	};

	using BenchmarkHistory = BenchmarkNode;

	struct BenchmarkLane
	{
		String name_;

		DynamicArray<BenchmarkEvent> events_;
	};

	struct BenchmarkCapture
	{
		Uint64 beginTicks_ = 0;

		Uint64 endTicks_ = 0;

		Double secondsPerTick_ = 0.0;

		DynamicArray<BenchmarkLane> lanes_;
	};

	class SEEDCORE_API BenchmarkSystem :public NonTransferable
	{
	public:
		BenchmarkSystem();

		~BenchmarkSystem();

		void Attach(String name);

		void Detach();

		void Collect();

		NODISCARD std::span<const BenchmarkNode> Nodes()const;

		NODISCARD std::span<const BenchmarkHistory> Histories()const;

		NODISCARD const BenchmarkFrames& Frames()const;

		NODISCARD const BenchmarkCapture& Capture()const;

		NODISCARD Bool Paused()const;

		void Paused(Bool paused);

	private:
		DynamicArray<ResourcePtr<BenchmarkThread>> threads_;

		DynamicArray<Size> roots_;

		std::mutex mutex_;

		DynamicArray<BenchmarkNode> nodes_;

		BenchmarkFrames frames_;

		BenchmarkCapture capture_;

		DynamicArray<Size> stack_;

		Uint64 lastCollectTicks_ = 0;

		Bool paused_ = false;
	};
}
