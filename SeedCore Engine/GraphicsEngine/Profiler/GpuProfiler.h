#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct RenderGraphStage;

	class RenderDevice;
	class D3D12CommandList;

	struct GpuProfilerResult
	{
		String featureName_;

		String stageName_;

		RenderViewMask view_ = RenderViewMask::Frame;

		Bool async_ = false;

		Float milliseconds_ = 0.0f;
	};

	class SEEDCORE_API GpuProfiler
	{
	public:
		void Create(RenderDevice& device);

		void Begin(D3D12CommandList& list, Uint32 pass, const RenderGraphStage& stage, RenderViewMask view);

		void End(D3D12CommandList& list, Uint32 pass);

		void Resolve(D3D12CommandList& list, Uint32 passCount);

		void Read();

		NODISCARD std::span<const GpuProfilerResult> Results()const;

		NODISCARD Bool Available()const;

	private:
		SC_CONSTEX Uint32 passCapacity_ = 1024;

		SC_CONSTEX Uint32 frameCount_ = 3;

		Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap_;

		Microsoft::WRL::ComPtr<ID3D12Resource> readback_;

		StaticArray<Uint64, 2> frequencies_ = { 0, 0 };

		StaticArray<DynamicArray<GpuProfilerResult>, frameCount_> records_;

		DynamicArray<GpuProfilerResult> results_;

		Uint32 writeFrame_ = 0;

		Uint64 resolveCount_ = 0;

		Bool warned_ = false;

		Bool available_ = false;
	};
}
