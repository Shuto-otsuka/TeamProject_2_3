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
	void GpuProfiler::Create(RenderDevice& device)
	{
		available_ = false;

		D3D12Context& context = device.Context();
		if (FAILED(context.GetDirectQueue()->GetCommandQueue()->GetTimestampFrequency(&frequencies_[0])) || FAILED(context.GetComputeQueue()->GetCommandQueue()->GetTimestampFrequency(&frequencies_[1])) || frequencies_[0] == 0 || frequencies_[1] == 0)
		{
			SC_LOG_WARNING("GPU のタイムスタンプの周波数を取得できませんでした。GPU プロファイラを無効にします。");
			return;
		}

		ID3D12Device* d3dDevice = context.GetDevice();
		D3D12_QUERY_HEAP_DESC queryHeapDesc{};
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queryHeapDesc.Count = passCapacity_ * 2;
		if (FAILED(d3dDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeap_))))
		{
			SC_LOG_WARNING("タイムスタンプのクエリのヒープを作れませんでした。GPU プロファイラを無効にします。");
			return;
		}

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

	void GpuProfiler::Begin(D3D12CommandList& list, Uint32 pass, const RenderGraphStage& stage, RenderViewMask view)
	{
		Benchmark::Begin(stage.stageName_);

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

		list.Get()->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, pass * 2);
	}

	void GpuProfiler::End(D3D12CommandList& list, Uint32 pass)
	{
		Benchmark::End(records_[writeFrame_][pass].stageName_);

		if (!available_ || pass >= passCapacity_)
		{
			return;
		}
		list.Get()->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, pass * 2 + 1);
	}

	void GpuProfiler::Resolve(D3D12CommandList& list, Uint32 passCount)
	{
		if (!available_)
		{
			return;
		}

		Uint32 timedCount = passCount < passCapacity_ ? passCount : passCapacity_;
		if (timedCount > 0)
		{
			list.Get()->ResolveQueryData(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, timedCount * 2, readback_.Get(), static_cast<Uint64>(writeFrame_) * passCapacity_ * 2 * sizeof(Uint64));
		}

		writeFrame_ = (writeFrame_ + 1) % frameCount_;
		++resolveCount_;
	}

	void GpuProfiler::Read()
	{
		if (!available_)
		{
			return;
		}

		DynamicArray<GpuProfilerResult>& records = records_[writeFrame_];

		if (resolveCount_ >= frameCount_)
		{
			results_.clear();
			Uint32 timedCount = records.size() < passCapacity_ ? static_cast<Uint32>(records.size()) : passCapacity_;
			Uint64 regionBegin = static_cast<Uint64>(writeFrame_) * passCapacity_ * 2 * sizeof(Uint64);

			D3D12_RANGE readRange{};
			readRange.Begin = static_cast<SIZE_T>(regionBegin);
			readRange.End = static_cast<SIZE_T>(regionBegin + static_cast<Uint64>(timedCount) * 2 * sizeof(Uint64));

			void* mapped = nullptr;
			if (timedCount > 0 && SUCCEEDED(readback_->Map(0, &readRange, &mapped)) && mapped)
			{
				const Uint64* timestamps = reinterpret_cast<const Uint64*>(static_cast<const Byte*>(mapped) + regionBegin);
				for (Uint32 index = 0;index < timedCount;++index)
				{
					GpuProfilerResult result = records[index];
					Uint64 beginTick = timestamps[index * 2];
					Uint64 endTick = timestamps[index * 2 + 1];

					Uint64 elapsed = endTick > beginTick ? endTick - beginTick : 0;
					result.milliseconds_ = static_cast<Float>(static_cast<Double>(elapsed) * 1000.0 / static_cast<Double>(frequencies_[result.async_ ? 1 : 0]));
					results_.push_back(result);
				}

				D3D12_RANGE writtenRange{};
				readback_->Unmap(0, &writtenRange);
			}
		}

		records.clear();
	}

	std::span<const GpuProfilerResult> GpuProfiler::Results()const
	{
		return results_;
	}

	Bool GpuProfiler::Available()const
	{
		return available_;
	}
}
