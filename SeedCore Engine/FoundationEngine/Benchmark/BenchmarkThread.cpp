#include <FoundationEngine/Benchmark/BenchmarkThread.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the buffer with its first chunk, under the given lane name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最初のチャンクを持つバッファを、指定のレーン名で作る。
	*/
	BenchmarkThread::BenchmarkThread(String name) :name_(name)
	{
		chunks_.emplace_back();
		chunks_.back().resize(4096);
		Rewind();
	}

	/**
	* [EN]
	* Moves writing on to the next chunk, creating it when none is left.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 書き込み先を次のチャンクへ移す。残っていなければ作る。
	*/
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

	/**
	* [EN]
	* Rewinds writing to the start of the first chunk, keeping every chunk allocated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 書き込み位置を最初のチャンクの先頭へ戻す。チャンクは確保したまま残す。
	*/
	void BenchmarkThread::Rewind()
	{
		chunkIndex_ = 0;
		write_ = chunks_[0].data();
		end_ = write_ + chunks_[0].size();
	}
}