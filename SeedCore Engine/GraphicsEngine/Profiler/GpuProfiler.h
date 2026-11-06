#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct RenderGraphStage;

	class RenderDevice;
	class D3D12CommandList;

	/**
	* [EN]
	* GPU time of one pass (one Stage run for one view) in a frame that has been read back.
	* The same struct also serves as the per-slot record written while the frame is being recorded, with milliseconds_ still zero,
	* so the names attached to a timestamp always come from the frame that wrote it even when the pass list changes between frames.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 読み戻したフレームでの、1 パス（1 つの Stage を 1 つのビューで動かしたもの）の GPU の時間。
	* 記録中のフレームでは、milliseconds_ が 0 のまま、スロットごとの記録としても同じ構造体を使う。
	* そのため、フレームごとにパスの並びが変わっても、タイムスタンプに付く名前は必ずそれを書いたフレームのものになる。
	*/
	struct GpuProfilerResult
	{
		/// [EN] Name of the Feature that owns the Stage, paired with stageName_ like RenderGraphStage.
		/// [JP] Stage を持つ Feature の名前。RenderGraphStage と同じく stageName_ と対になる。
		String featureName_;

		/// [EN] Name of the Stage ("<Feature>.<Verb>").
		/// [JP] Stage の名前（"<Feature>.<動詞>"）。
		String stageName_;

		/// [EN] View the pass ran for: exactly one of Editor / Game / Canvas, or Frame for a pass that runs once per frame.
		/// [JP] パスが動いたビュー。Editor / Game / Canvas のどれか 1 つで、フレームで 1 回のパスは Frame。
		RenderViewMask view_ = RenderViewMask::Frame;

		/// [EN] Whether the pass ran on the asynchronous Compute queue. Such a pass overlaps the Direct queue, so adding it to the other passes does not give the frame time.
		/// [JP] 非同期の Compute キューで動いたか。このパスは Direct キューと重なって走るので、他のパスと足してもフレームの時間にはならない。
		Bool async_ = false;

		/// [EN] GPU time between the timestamps written before and after the pass.
		/// [JP] パスの前後で書いたタイムスタンプの間の GPU の時間。
		Float milliseconds_ = 0.0f;
	};

	/**
	* [EN]
	* Per-pass GPU timing for the RenderGraph, using D3D12 timestamp queries.
	* RenderGraph::Execute brackets every pass with Begin / End, so nothing is listed or wrapped by hand: a new Stage is timed as soon as it is added to the graph.
	*
	* Timestamps are written on both the Direct queue and the asynchronous Compute queue. Each queue reports its own tick frequency, so every record keeps
	* which queue it ran on and is converted with that queue's frequency.
	*
	* The query heap holds one frame of slots (two per pass). Resolve is recorded on the Direct queue at the end of Execute, after the Direct queue has waited
	* for the Compute queue, so the timestamps of both queues are complete when they are copied out. The next frame's queries are recorded after that resolve
	* in GPU order, which makes reusing the same slots every frame safe.
	*
	* The readback buffer and the per-slot records form a ring of frameCount_ frames. Read reads the region that is about to be reused, which was resolved
	* frameCount_ frames earlier and is therefore finished on the GPU, then clears its records for the frame that is starting.
	*
	* Every pass is also a CPU section of the Benchmark under the Stage name, so the cost of recording a pass shows up next to its GPU time.
	*
	* SEEDCORE_API is needed because the Editor reads Results across the SeedCore.Cplusplus.dll boundary.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* D3D12 のタイムスタンプのクエリを使った、RenderGraph のパスごとの GPU の計測。
	* RenderGraph::Execute がすべてのパスを Begin / End で囲むので、計る区間を手で並べたり囲んだりしない。Stage をグラフに足した時点で計測される。
	*
	* タイムスタンプは Direct キューと非同期の Compute キューの両方で書く。周波数はキューごとに違いうるので、記録ごとにどちらのキューで走ったかを残し、
	* そのキューの周波数で時間に直す。
	*
	* クエリのヒープは 1 フレーム分のスロット（パスごとに 2 つ）だけを持つ。Resolve は Execute の最後、Direct キューが Compute キューを待った後に
	* Direct キューで記録するので、両方のキューのタイムスタンプが書き終わってから写される。次のフレームのクエリは GPU の順番でその Resolve の後に
	* 記録されるので、毎フレーム同じスロットを使い回してよい。
	*
	* 読み戻しのバッファとスロットごとの記録は frameCount_ フレームのリングにする。Read は次に使い回す領域を読む。そこは frameCount_ フレーム前に
	* Resolve したもので、GPU は確実に書き終えている。読んだ後、始まるフレームのためにその記録を空にする。
	*
	* 各パスは Stage の名前で Benchmark の CPU の区間にもするので、パスの記録にかかる CPU の時間が GPU の時間と並んで見える。
	*
	* SEEDCORE_API が要るのは、Editor が SeedCore.Cplusplus.dll の外から Results を読むため。
	*/
	class SEEDCORE_API GpuProfiler
	{
	public:
		/**
		* [EN]
		* Creates the query heap and the ringed readback buffer, and reads the tick frequency of the Direct and Compute queues.
		* On failure the profiler stays unavailable and every other function does nothing, so the graph never has to check before calling them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クエリのヒープとリングにした読み戻しのバッファを作り、Direct キューと Compute キューの周波数を読む。
		* 失敗したときは使えないままにし、ほかの関数は何もしない。そのためグラフは呼ぶ前に確かめなくてよい。
		*/
		void Create(RenderDevice& device);

		/**
		* [EN]
		* Writes the timestamp before a pass and records which Stage, view and queue the pass's slots belong to.
		* Passes beyond passCapacity_ are not timed, and a warning is logged once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスの前のタイムスタンプを書き、そのパスのスロットがどの Stage・ビュー・キューのものかを記録する。
		* passCapacity_ を超えたパスは計らず、警告を 1 回だけ出す。
		*/
		void Begin(D3D12CommandList& list, Uint32 pass, const RenderGraphStage& stage, RenderViewMask view);

		/**
		* [EN]
		* Writes the timestamp after a pass, on the same command list that Begin used.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスの後のタイムスタンプを、Begin と同じコマンドリストで書く。
		*/
		void End(D3D12CommandList& list, Uint32 pass);

		/**
		* [EN]
		* Copies the timestamps of the passes recorded this frame into this frame's region of the readback buffer, then moves the ring to the next frame.
		* Called on the Direct command list after the Direct queue has waited for the Compute queue.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このフレームで記録したパスのタイムスタンプを、読み戻しのバッファのこのフレームの領域へ写し、リングを次のフレームへ進める。
		* Direct キューが Compute キューを待った後に、Direct のコマンドリストで呼ぶ。
		*/
		void Resolve(D3D12CommandList& list, Uint32 passCount);

		/**
		* [EN]
		* Reads back the oldest resolved frame and rebuilds Results from its records, then clears those records for the frame that is starting.
		* Called once at the start of Execute.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Resolve 済みのいちばん古いフレームを読み戻して、その記録から Results を作り直し、始まるフレームのためにその記録を空にする。
		* Execute の始めに 1 回呼ぶ。
		*/
		void Read();

		/**
		* [EN]
		* GPU time of every pass in the most recently read frame, in the order the passes ran.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最後に読み戻したフレームの、全パスの GPU の時間。パスが動いた順に並ぶ。
		*/
		NODISCARD std::span<const GpuProfilerResult> Results()const;

		/**
		* [EN]
		* Whether Create succeeded and timestamps are being written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Create が成功し、タイムスタンプを書いているか。
		*/
		NODISCARD Bool Available()const;

	private:
		/// [EN] Number of passes that can be timed in one frame. Each pass uses two timestamp slots (before and after).
		/// [JP] 1 フレームで計れるパスの数。パスごとにタイムスタンプのスロットを 2 つ（前と後）使う。
		SC_CONSTEX Uint32 passCapacity_ = 1024;

		/// [EN] Length of the readback ring. Three frames keep the region being read at least two frames behind the GPU while two frames are in flight.
		/// [JP] 読み戻しのリングの長さ。GPU が 2 フレーム先まで走るので、3 フレームにすると読む領域は常に GPU より 2 フレーム以上遅れる。
		SC_CONSTEX Uint32 frameCount_ = 3;

		/// [EN] Timestamp query heap holding one frame of slots.
		/// [JP] 1 フレーム分のスロットを持つ、タイムスタンプのクエリのヒープ。
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap_;

		/// [EN] READBACK buffer holding frameCount_ regions, one frame of slots each.
		/// [JP] 1 フレーム分のスロットの領域を frameCount_ 個並べた、READBACK のバッファ。
		Microsoft::WRL::ComPtr<ID3D12Resource> readback_;

		/// [EN] Tick frequency of the Direct queue (index 0) and the Compute queue (index 1), matching the queue numbers used by RenderGraph.
		/// [JP] Direct キュー（0）と Compute キュー（1）の周波数。RenderGraph のキューの番号と同じ並び。
		StaticArray<Uint64, 2> frequencies_ = { 0, 0 };

		/// [EN] Per-slot records of each frame in the ring, indexed by pass. milliseconds_ stays zero here.
		/// [JP] リングの各フレームの、パスの番号で引くスロットごとの記録。ここでは milliseconds_ は 0 のまま。
		StaticArray<DynamicArray<GpuProfilerResult>, frameCount_> records_;

		/// [EN] Results of the most recently read frame.
		/// [JP] 最後に読み戻したフレームの結果。
		DynamicArray<GpuProfilerResult> results_;

		/// [EN] Ring frame that the current frame records into and resolves to.
		/// [JP] 今のフレームが記録し、Resolve で写す先のリングのフレーム。
		Uint32 writeFrame_ = 0;

		/// [EN] Number of frames resolved so far. Read does nothing until a full ring has been resolved.
		/// [JP] これまでに Resolve したフレームの数。リング 1 周分を Resolve するまで Read は何も読まない。
		Uint64 resolveCount_ = 0;

		/// [EN] Whether the warning for exceeding passCapacity_ has already been logged.
		/// [JP] passCapacity_ を超えたときの警告をもう出したか。
		Bool warned_ = false;

		/// [EN] Whether Create succeeded.
		/// [JP] Create が成功したか。
		Bool available_ = false;
	};
}
