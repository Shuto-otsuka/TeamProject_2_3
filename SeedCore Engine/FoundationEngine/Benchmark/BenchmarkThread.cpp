#include <FoundationEngine/Benchmark/BenchmarkThread.h>

namespace SeedCore
{
	BenchmarkThread::BenchmarkThread()
	{
		chunks_.emplace_back();
		chunks_.back().resize(4096);
		Rewind();
	}

	void BenchmarkThread::Grow()
	{
		++chunkIndex_;
		if (chunkIndex_ == chunks_.size())
		{
			chunks_.emplace_back();
			chunks_.back().resize(4096);
		}
		write_ = chunks_[chunkIndex_].data();
		end_ = write_ + chunks_[chunkIndex_].size();
	}

	void BenchmarkThread::Rewind()
	{
		chunkIndex_ = 0;
		write_ = chunks_[0].data();
		end_ = write_ + chunks_[0].size();
	}
}