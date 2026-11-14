#include <GraphicsEngine/Profiler/GpuProfiler.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
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
	void GpuProfiler::Create(RenderDevice& device)
	{
		available_ = false;

		/// [EN] Each queue reports its own tick frequency, so both are read and kept in the order of the graph's queue numbers.
		/// [JP] 周波数はキューごとに違いうるので、両方を読み、グラフのキューの番号の順に持つ。
		D3D12Context& context = device.Context();
		if (FAILED(context.GetDirectQueue()->GetCommandQueue()->GetTimestampFrequency(&frequencies_[0])) || FAILED(context.GetComputeQueue()->GetCommandQueue()->GetTimestampFrequency(&frequencies_[1])) || frequencies_[0] == 0 || frequencies_[1] == 0)
		{
			SC_LOG_WARNING("GPU のタイムスタンプの周波数を取得できませんでした。GPU プロファイラを無効にします。");
			return;
		}

		/// [EN] The query heap holds one frame of slots: two per pass (before and after).
		/// [JP] クエリのヒープは 1 フレーム分のスロットを持つ。パスごとに 2 つ（前と後）。
		ID3D12Device* d3dDevice = context.GetDevice();
		D3D12_QUERY_HEAP_DESC queryHeapDesc{};
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queryHeapDesc.Count = passCapacity_ * 2;
		if (FAILED(d3dDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeap_))))
		{
			SC_LOG_WARNING("タイムスタンプのクエリのヒープを作れませんでした。GPU プロファイラを無効にします。");
			return;
		}

		/// [EN] The readback buffer holds frameCount_ regions of one frame each, so the region being read is never the one the GPU is copying into.
		/// [JP] 読み戻しのバッファは 1 フレーム分の領域を frameCount_ 個持つ。そのため読む領域が、GPU が写している領域と重ならない。
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = static_cast<Uint64>(frameCount_) * passCapacity_ * 2 * sizeof(Uint64);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		/// [EN] A READBACK buffer stays in COPY_DEST for its whole life, which is the state ResolveQueryData writes into.
		/// [JP] READBACK のバッファはずっと COPY_DEST のままにする。ResolveQueryData が書き込む状態がそれ。
		if (FAILED(d3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback_))))
		{
			SC_LOG_WARNING("タイムスタンプの読み戻しのバッファを作れませんでした。GPU プロファイラを無効にします。");
			queryHeap_.Reset();
			return;
		}
#if SC_DEBUG
		readback_->SetName(L"GpuProfiler.Readback");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(readback_.Get());
#endif

		/// [EN] Start the ring from an empty state.
		/// [JP] リングを空の状態から始める。
		for (DynamicArray<GpuProfilerResult>& records : records_)
		{
			records.clear();
		}
		results_.clear();
		writeFrame_ = 0;
		resolveCount_ = 0;
		warned_ = false;
		available_ = true;
	}

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
	void GpuProfiler::Begin(D3D12CommandList& list, Uint32 pass, const RenderGraphStage& stage, RenderViewMask view)
	{
		/// [EN] Every pass is also a CPU section under its Stage name, recorded even when timestamps are unavailable.
		/// [JP] 各パスは Stage の名前で CPU の区間にもする。タイムスタンプが使えないときも記録する。
		Benchmark::Begin(stage.stageName_);

		/// [EN] The record is kept for every pass, so End can close the CPU section by name even for a pass that is not timed.
		/// [JP] 記録はすべてのパスについて残す。計らないパスでも、End が名前で CPU の区間を閉じられるようにするため。
		DynamicArray<GpuProfilerResult>& records = records_[writeFrame_];
		if (records.size() <= pass)
		{
			records.resize(pass + 1);
		}
		records[pass] = GpuProfilerResult{ stage.featureName_, stage.stageName_, view, stage.async_, 0.0f };

		if (!available_)
		{
			return;
		}
		if (pass >= passCapacity_)
		{
			if (!warned_)
			{
				SC_LOG_WARNING("GPU プロファイラ: 1 フレームのパスが {} を超えたので、超えた分は計りません", passCapacity_);
				warned_ = true;
			}
			return;
		}

		/// [EN] A timestamp query has no Begin: EndQuery alone writes the current GPU time into the slot.
		/// [JP] タイムスタンプのクエリには Begin が無く、EndQuery だけでそのときの GPU の時刻をスロットに書く。
		list.Get()->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, pass * 2);
	}

	/**
	* [EN]
	* Writes the timestamp after a pass, on the same command list that Begin used.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パスの後のタイムスタンプを、Begin と同じコマンドリストで書く。
	*/
	void GpuProfiler::End(D3D12CommandList& list, Uint32 pass)
	{
		/// [EN] Close the CPU section opened by Begin, using the name Begin recorded.
		/// [JP] Begin が開いた CPU の区間を、Begin が記録した名前で閉じる。
		Benchmark::End(records_[writeFrame_][pass].stageName_);

		if (!available_ || pass >= passCapacity_)
		{
			return;
		}
		list.Get()->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, pass * 2 + 1);
	}

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
	void GpuProfiler::Resolve(D3D12CommandList& list, Uint32 passCount)
	{
		if (!available_)
		{
			return;
		}

		/// [EN] Only the slots written this frame are resolved. Resolving a slot that was never written gives undefined data, and the debug layer warns about it.
		/// [JP] このフレームで書いたスロットだけを写す。一度も書いていないスロットを写すと中身が未定義になり、デバッグレイヤも警告を出す。
		Uint32 timedCount = passCount < passCapacity_ ? passCount : passCapacity_;
		if (timedCount > 0)
		{
			list.Get()->ResolveQueryData(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, timedCount * 2, readback_.Get(), static_cast<Uint64>(writeFrame_) * passCapacity_ * 2 * sizeof(Uint64));
		}

		/// [EN] The next frame records into the next region of the ring.
		/// [JP] 次のフレームは、リングの次の領域に記録する。
		writeFrame_ = (writeFrame_ + 1) % frameCount_;
		++resolveCount_;
	}

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
	void GpuProfiler::Read()
	{
		if (!available_)
		{
			return;
		}

		/// [EN] The region the starting frame is about to reuse was resolved frameCount_ frames ago, so the GPU has finished writing it.
		/// [JP] 始まるフレームが使い回す領域は frameCount_ フレーム前に写したものなので、GPU は書き終えている。
		DynamicArray<GpuProfilerResult>& records = records_[writeFrame_];

		/// [EN] Until a full ring has been resolved, the region holds nothing yet and the previous results are kept.
		/// [JP] リング 1 周分を写すまでは領域にまだ何も入っていないので、前の結果をそのまま残す。
		if (resolveCount_ >= frameCount_)
		{
			results_.clear();
			Uint32 timedCount = records.size() < passCapacity_ ? static_cast<Uint32>(records.size()) : passCapacity_;
			Uint64 regionBegin = static_cast<Uint64>(writeFrame_) * passCapacity_ * 2 * sizeof(Uint64);

			/// [EN] Map only the range that is read, so the driver knows which part has to be coherent with the CPU.
			/// [JP] 読む範囲だけを Map し、CPU と整合させる範囲をドライバに伝える。
			D3D12_RANGE readRange{};
			readRange.Begin = static_cast<SIZE_T>(regionBegin);
			readRange.End = static_cast<SIZE_T>(regionBegin + static_cast<Uint64>(timedCount) * 2 * sizeof(Uint64));

			void* mapped = nullptr;
			if (timedCount > 0 && SUCCEEDED(readback_->Map(0, &readRange, &mapped)) && mapped)
			{
				const Uint64* timestamps = reinterpret_cast<const Uint64*>(static_cast<const Byte*>(mapped) + regionBegin);
				for (Uint32 index = 0;index < timedCount;++index)
				{
					/// [EN] Copy the record and fill in its time. The tick frequency is the one of the queue the pass ran on.
					/// [JP] 記録を写して時間を入れる。周波数は、そのパスが走ったキューのもの。
					GpuProfilerResult result = records[index];
					Uint64 beginTick = timestamps[index * 2];
					Uint64 endTick = timestamps[index * 2 + 1];

					/// [EN] An end tick that is not after the begin tick cannot be measured and counts as zero; subtracting it as unsigned would wrap to a huge value.
					/// [JP] 終わりが始めより後でない時刻は計れないので 0 とする。符号なしで引くと巨大な値になるため。
					Uint64 elapsed = endTick > beginTick ? endTick - beginTick : 0;
					result.milliseconds_ = static_cast<Float>(static_cast<Double>(elapsed) * 1000.0 / static_cast<Double>(frequencies_[result.async_ ? 1 : 0]));
					results_.push_back(result);
				}

				/// [EN] Nothing was written by the CPU, so the written range is empty.
				/// [JP] CPU は何も書いていないので、書いた範囲は空にする。
				D3D12_RANGE writtenRange{};
				readback_->Unmap(0, &writtenRange);
			}
		}

		/// [EN] The starting frame records into this region from now on.
		/// [JP] 始まるフレームは、これからこの領域に記録する。
		records.clear();
	}

	/**
	* [EN]
	* GPU time of every pass in the most recently read frame, in the order the passes ran.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最後に読み戻したフレームの、全パスの GPU の時間。パスが動いた順に並ぶ。
	*/
	std::span<const GpuProfilerResult> GpuProfiler::Results()const
	{
		return results_;
	}

	/**
	* [EN]
	* Whether Create succeeded and timestamps are being written.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Create が成功し、タイムスタンプを書いているか。
	*/
	Bool GpuProfiler::Available()const
	{
		return available_;
	}
}
