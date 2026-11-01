#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* A section that has begun on a thread and not yet ended.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* あるスレッドで始まり、まだ終わっていない区間。
	*/
	struct BenchmarkOpenSection
	{
		/// [EN] Section name literal.
		/// [JP] 区間名のリテラル。
		const Char* name_ = nullptr;

		/// [EN] Timestamp tick at Begin.
		/// [JP] Begin 時点のタイムスタンプ tick。
		Uint64 startTicks_ = 0;
	};

	/**
	* [EN]
	* One finished section. The start tick and the nesting depth let the
	* sections of a frame be rebuilt into a call tree and laid out on a
	* timeline.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 終わった区間1つ。開始 tick と入れ子の深さから、フレームの区間を
	* 呼び出しツリーに組み直し、タイムラインに並べられる。
	*/
	struct BenchmarkEvent
	{
		/// [EN] Section name literal.
		/// [JP] 区間名のリテラル。
		const Char* name_ = nullptr;

		/// [EN] Timestamp tick at Begin.
		/// [JP] Begin 時点のタイムスタンプ tick。
		Uint64 startTicks_ = 0;

		/// [EN] Ticks between Begin and End.
		/// [JP] Begin から End までの tick 数。
		Uint64 ticks_ = 0;

		/// [EN] Number of sections still open around this one when it ended; 0 is a top-level section.
		/// [JP] この区間が終わった時点で外側に開いていた区間の数。0 は最上位の区間。
		Uint32 depth_ = 0;
	};

	/**
	* [EN]
	* Per-thread event buffer. The owning thread appends events while
	* BenchmarkSystem drains them from the main thread, so appending and
	* draining are serialized by a spin lock that is only ever held for the
	* length of one append or one copy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スレッドごとのイベントバッファ。持ち主のスレッドがイベントを書き足す一方、
	* BenchmarkSystem がメインスレッドから吸い出すので、書き足しと吸い出しを
	* スピンロックで直列化する。ロックを持つのは1回の書き足しか1回のコピーの
	* 間だけ。
	*/
	class SEEDCORE_API BenchmarkThread :public NonTransferable
	{
	private:
		friend class Benchmark;
		friend class BenchmarkSystem;

	public:
		/**
		* [EN]
		* Creates the buffer with its first chunk, under the given lane name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最初のチャンクを持つバッファを、指定のレーン名で作る。
		*/
		BenchmarkThread(String name);

	private:
		/**
		* [EN]
		* Moves writing on to the next chunk, creating it when none is left.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 書き込み先を次のチャンクへ移す。残っていなければ作る。
		*/
		__declspec(noinline) void Grow();

		/**
		* [EN]
		* Rewinds writing to the start of the first chunk, keeping every chunk allocated.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 書き込み位置を最初のチャンクの先頭へ戻す。チャンクは確保したまま残す。
		*/
		void Rewind();

	private:
		/// [EN] Number of sections currently open on this thread.
		/// [JP] このスレッドで今開いている区間の数。
		alignas(64) Size openCount_ = 0;

		/// [EN] Next event slot in the current chunk.
		/// [JP] 今のチャンクで次にイベントを書く位置。
		BenchmarkEvent* write_ = nullptr;

		/// [EN] End of the current chunk.
		/// [JP] 今のチャンクの終端。
		BenchmarkEvent* end_ = nullptr;

		/// [EN] Index of the chunk being written.
		/// [JP] 書き込み中のチャンクの添字。
		Size chunkIndex_ = 0;

		/// [EN] Begins ignored because the open stack was full; their Ends are ignored too.
		/// [JP] 開いている区間のスタックが満杯で無視した Begin の数。対応する End も無視する。
		Size overflowDepth_ = 0;

		/// [EN] Ends whose name did not match the innermost open section.
		/// [JP] 名前が一番内側の開いている区間と合わなかった End の数。
		Size mismatchCount_ = 0;

		/// [EN] Ends with no open section.
		/// [JP] 開いている区間が無いのに来た End の数。
		Size unmatchedEndCount_ = 0;

		/// [EN] Held while an event is appended or the buffer is drained.
		/// [JP] イベントの書き足し中と、バッファの吸い出し中に持つ。
		std::atomic_flag lock_;

		/// [EN] Lane name shown on the timeline and as the root of the call tree.
		/// [JP] タイムラインと呼び出しツリーの根に出すレーン名。
		String name_;

		/// [EN] Sections currently open, innermost last.
		/// [JP] 今開いている区間。一番内側が末尾。
		StaticArray<BenchmarkOpenSection, 64> open_{};

		/// [EN] Event storage; chunks are kept across frames so steady-state recording never allocates.
		/// [JP] イベントの格納先。チャンクはフレームをまたいで残し、定常状態の記録で確保が起きないようにする。
		DynamicArray<DynamicArray<BenchmarkEvent>> chunks_;
	};
}