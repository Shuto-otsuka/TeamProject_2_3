#pragma once
#include <FoundationEngine/Benchmark/BenchmarkThread.h>

namespace SeedCore
{
	/**
	* [EN]
	* Ring of per-frame wall-clock times. Every per-frame ring in the
	* benchmark (frame times and node times) shares this ring's slot index,
	* so slot next_ - 1 of any of them belongs to the same frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームごとの実時間のリング。ベンチマーク内のフレームごとのリング
	* （フレーム時間とノードの時間）は全てこのリングの添字を共有するので、
	* どのリングでも next_ - 1 番目は同じフレームのものになる。
	*/
	struct BenchmarkFrames
	{
		/// [EN] Number of frames every ring holds.
		/// [JP] 各リングが持つフレーム数。
		SC_CONSTEX Size capacity_ = 300;

		/// [EN] Time from one Collect to the next, i.e. the length of each frame.
		/// [JP] Collect から次の Collect までの時間。つまり各フレームの長さ。
		StaticArray<Double, capacity_> seconds_{};

		/// [EN] Frames recorded so far; only the last capacity_ remain.
		/// [JP] これまでに記録したフレーム数。残るのは直近 capacity_ 個だけ。
		Size count_ = 0;

		/// [EN] Slot the next frame is written to.
		/// [JP] 次のフレームを書き込む位置。
		Size next_ = 0;
	};

	/**
	* [EN]
	* One node of the call tree: a section reached through one particular
	* chain of enclosing sections, so the same name under two different
	* parents is two nodes. A depth-0 node is the root of one thread and
	* holds that thread's busy time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼び出しツリーのノード1つ。外側の区間の連なり1通りごとの区間なので、
	* 同じ名前でも親が違えば別ノードになる。深さ 0 のノードは1スレッドの根で、
	* そのスレッドが働いていた時間を持つ。
	*/
	struct BenchmarkNode
	{
		/// [EN] Section name, or the thread's lane name for a root.
		/// [JP] 区間名。根ならスレッドのレーン名。
		String name_;

		/// [EN] Literal address the node was first found by, compared before the name for speed.
		/// [JP] ノードを最初に見つけたリテラルのアドレス。速さのため名前より先に比べる。
		const Char* key_ = nullptr;

		/// [EN] 0 for a thread root, 1 for its top-level sections, and so on.
		/// [JP] スレッドの根が 0、その最上位の区間が 1、以下同様。
		Uint32 depth_ = 0;

		/// [EN] Indices of the child nodes, in the order they first appeared.
		/// [JP] 子ノードの添字。最初に現れた順。
		DynamicArray<Size> children_;

		/// [EN] Total seconds per frame, indexed like BenchmarkFrames.
		/// [JP] フレームごとの合計秒数。添字は BenchmarkFrames と共通。
		StaticArray<Double, BenchmarkFrames::capacity_> seconds_{};

		/// [EN] Calls per frame, indexed like BenchmarkFrames.
		/// [JP] フレームごとの呼び出し回数。添字は BenchmarkFrames と共通。
		StaticArray<Uint32, BenchmarkFrames::capacity_> calls_{};

		/// [EN] Number of valid samples in the shared rings.
		/// [JP] 共有リング内の有効サンプル数。
		Size count_ = 0;

		/// [EN] Slot that will receive the next sample.
		/// [JP] 次のサンプルを書き込む枠。
		Size next_ = 0;
	};

	/// [EN] Legacy flat-table view of a call-tree node.
	/// [JP] 呼び出しツリーノードの旧フラット表向けビュー。
	using BenchmarkHistory = BenchmarkNode;

	/**
	* [EN]
	* Every event one thread recorded in the captured frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 取り込んだフレームで1スレッドが記録した全イベント。
	*/
	struct BenchmarkLane
	{
		/// [EN] Thread name shown at the head of the lane.
		/// [JP] レーンの先頭に出すスレッド名。
		String name_;

		/// [EN] Events sorted by start tick, parents before their children.
		/// [JP] 開始 tick 順に並べたイベント。親は子より前に来る。
		DynamicArray<BenchmarkEvent> events_;
	};

	/**
	* [EN]
	* Raw events of the latest collected frame, for the timeline. Only one
	* frame is kept, since keeping every frame's events would cost far more
	* memory than the per-node rings.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイムライン用の、最後に集計したフレームの生イベント。全フレーム分を
	* 残すとノードのリングよりはるかにメモリを食うので、1フレーム分だけ持つ。
	*/
	struct BenchmarkCapture
	{
		/// [EN] Tick of the Collect that started the frame.
		/// [JP] フレームを始めた Collect の tick。
		Uint64 beginTicks_ = 0;

		/// [EN] Tick of the Collect that ended the frame.
		/// [JP] フレームを終えた Collect の tick。
		Uint64 endTicks_ = 0;

		/// [EN] Length of one tick in seconds, for converting the events.
		/// [JP] 1 tick の秒数。イベントの換算に使う。
		Double secondsPerTick_ = 0.0;

		/// [EN] One lane per attached thread, in attach order.
		/// [JP] Attach したスレッドごとに1レーン。Attach した順。
		DynamicArray<BenchmarkLane> lanes_;
	};

	/**
	* [EN]
	* Owner of the CPU section measurements. Benchmark::Begin/End only append
	* raw events to the calling thread's BenchmarkThread; once per frame this
	* class drains every thread, rebuilds the frame's call tree into the
	* per-node rings, and keeps the frame's raw events for the timeline.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CPU 区間計測の持ち主。Benchmark::Begin/End は呼んだスレッドの
	* BenchmarkThread に生のイベントを書き足すだけで、このクラスがフレームに
	* 1回全スレッドから吸い出し、そのフレームの呼び出しツリーをノードごとの
	* リングに組み直し、タイムライン用にフレームの生イベントを残す。
	*/
	class SEEDCORE_API BenchmarkSystem :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Measures how many seconds one CPU timestamp tick lasts.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CPU のタイムスタンプ 1 tick が何秒にあたるかを測る。
		*/
		BenchmarkSystem();

		/**
		* [EN]
		* Unbinds the calling thread from its event buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼んだスレッドをイベントバッファから外す。
		*/
		~BenchmarkSystem();

		/**
		* [EN]
		* Gives the calling thread its own event buffer and call-tree root
		* under the given name, so its Benchmark::Begin/End calls start being
		* recorded. Safe to call from any thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼んだスレッドに、指定の名前で専用のイベントバッファと呼び出しツリーの
		* 根を持たせ、そのスレッドの Benchmark::Begin/End が記録されるように
		* する。どのスレッドから呼んでもよい。
		*/
		void Attach(String name);

		/**
		* [EN]
		* Unbinds the calling thread, so its Benchmark::Begin/End calls are ignored.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼んだスレッドを外し、そのスレッドの Benchmark::Begin/End を無視させる。
		*/
		void Detach();

		/**
		* [EN]
		* Ends the frame: drains every thread's events, and unless paused,
		* folds them into the node rings and keeps them as the capture. Call
		* once per frame from the main thread, outside every section.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレームを締める。全スレッドのイベントを吸い出し、一時停止中でなければ
		* ノードのリングへまとめ、取り込みとして残す。メインスレッドから、どの区間の
		* 外側でもフレームに1回呼ぶ。
		*/
		void Collect();

		/**
		* [EN]
		* Every node of the call tree; depth-0 nodes are the thread roots.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼び出しツリーの全ノード。深さ 0 のノードがスレッドの根。
		*/
		NODISCARD std::span<const BenchmarkNode> Nodes()const;

		/**
		* [EN]
		* Returns the call-tree nodes through the former flat-history interface.
		* This lets existing clients migrate their presentation incrementally.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 以前のフラット履歴インターフェースとして呼び出しツリーノードを返す。
		* 既存の表示は段階的に移行できる。
		*/
		NODISCARD std::span<const BenchmarkHistory> Histories()const;

		/**
		* [EN]
		* Per-frame times, whose slot index the node rings share.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレームごとの時間。ノードのリングはこの添字を共有する。
		*/
		NODISCARD const BenchmarkFrames& Frames()const;

		/**
		* [EN]
		* Raw events of the latest collected frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最後に集計したフレームの生イベント。
		*/
		NODISCARD const BenchmarkCapture& Capture()const;

		/**
		* [EN]
		* Whether recording is paused.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 記録を一時停止しているかどうか。
		*/
		NODISCARD Bool Paused()const;

		/**
		* [EN]
		* Pauses or resumes recording. While paused, events are still drained
		* so the thread buffers do not grow, but the rings and the capture are
		* left untouched, so the frames on screen can be inspected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 記録を一時停止・再開する。停止中もスレッドのバッファが膨らまないよう
		* イベントは吸い出すが、リングと取り込みには触れず、画面上のフレームを
		* じっくり見られるようにする。
		*/
		void Paused(Bool paused);

	private:
		/// [EN] Event buffer of every attached thread.
		/// [JP] Attach した全スレッドのイベントバッファ。
		DynamicArray<ResourcePtr<BenchmarkThread>> threads_;

		/// [EN] Call-tree root node of each thread, parallel to threads_.
		/// [JP] 各スレッドの呼び出しツリーの根ノード。threads_ と同じ添字。
		DynamicArray<Size> roots_;

		/// [EN] Guards the threads, nodes and capture against Attach from a worker during Collect.
		/// [JP] Collect 中にワーカーから Attach されても、スレッド・ノード・取り込みが壊れないよう守る。
		std::mutex mutex_;

		/// [EN] Every node of the call tree.
		/// [JP] 呼び出しツリーの全ノード。
		DynamicArray<BenchmarkNode> nodes_;

		/// [EN] Per-frame times.
		/// [JP] フレームごとの時間。
		BenchmarkFrames frames_;

		/// [EN] Raw events of the latest collected frame.
		/// [JP] 最後に集計したフレームの生イベント。
		BenchmarkCapture capture_;

		/// [EN] Node of each open depth while one lane is folded into the tree; kept as a member so Collect does not allocate.
		/// [JP] 1レーンをツリーへまとめている間の、開いている各深さのノード。Collect で確保が起きないようメンバーに持つ。
		DynamicArray<Size> stack_;

		/// [EN] Tick of the previous Collect, the start of the frame being collected.
		/// [JP] 前回の Collect の tick。集計中のフレームの始まり。
		Uint64 lastCollectTicks_ = 0;

		/// [EN] Whether recording is paused.
		/// [JP] 記録を一時停止しているかどうか。
		Bool paused_ = false;
	};
}
