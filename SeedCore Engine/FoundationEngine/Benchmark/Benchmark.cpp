#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Benchmark/BenchmarkThread.h>

namespace SeedCore
{
	namespace
	{
		/// [EN] Buffer of the calling thread, or nullptr when the thread is not attached.
		/// [JP] 呼んだスレッドのバッファ。Attach されていなければ nullptr。
		constinit thread_local BenchmarkThread* currentThread = nullptr;
	}

	/**
	* [EN]
	* Opens a section whose name is decided at run time, such as a
	* component type name. An interned String keeps its address for the
	* life of the pool, so it is as safe to hold as a literal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネントの型名など、実行時に決まる名前で区間を開く。インターン
	* 済みの String はプールが生きている間アドレスが変わらないので、リテラルと
	* 同じく保持して安全。
	*/
	void Benchmark::Begin(const String& name)
	{
		BeginLiteral(name.c_str());
	}

	/**
	* [EN]
	* Closes the innermost section opened with the same String.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 同じ String で開いた一番内側の区間を閉じる。
	*/
	void Benchmark::End(const String& name)
	{
		EndLiteral(name.c_str());
	}

	/**
	* [EN]
	* Pushes the section onto the calling thread's open stack with the current tick.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼んだスレッドの開いている区間のスタックへ、今の tick と一緒に積む。
	*/
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

		/// [EN] The signal fences keep the compiler from moving the measured code across the timestamp read.
		/// [JP] シグナルフェンスで、計測対象のコードがコンパイラによってタイムスタンプの読み取りをまたいで動かされないようにする。
		std::atomic_signal_fence(std::memory_order_seq_cst);
		thread->open_[thread->openCount_++] = BenchmarkOpenSection{ name,__rdtsc() };
		std::atomic_signal_fence(std::memory_order_seq_cst);
	}

	/**
	* [EN]
	* Pops the innermost section and appends it as a finished event.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 一番内側の区間を降ろし、終わったイベントとして書き足す。
	*/
	void Benchmark::EndLiteral(const Char* name)
	{
		/// [EN] The end tick is read first, so the bookkeeping below is not counted in the section.
		/// [JP] 終了 tick を最初に読み、以下の後始末を区間に含めないようにする。
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

		/// [EN] The lock is only contended while BenchmarkSystem copies this thread's events out, once per frame.
		/// [JP] ロックが競合するのは、BenchmarkSystem がこのスレッドのイベントを写し出すフレームに1回の間だけ。
		while (thread->lock_.test_and_set(std::memory_order_acquire))
		{
			_mm_pause();
		}

		if (thread->write_ == thread->end_) [[unlikely]]
		{
			thread->Grow();
		}
		*thread->write_++ = BenchmarkEvent{ name,open.startTicks_,endTicks - open.startTicks_,static_cast<Uint32>(thread->openCount_) };

		thread->lock_.clear(std::memory_order_release);
	}

	/**
	* [EN]
	* Binds the calling thread to its buffer, or unbinds it with nullptr.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼んだスレッドをバッファに結び付ける。nullptr なら外す。
	*/
	void Benchmark::Bind(BenchmarkThread* thread)
	{
		currentThread = thread;
	}
}