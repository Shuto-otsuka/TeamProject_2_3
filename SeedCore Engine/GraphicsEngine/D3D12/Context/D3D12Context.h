#pragma once

namespace SeedCore
{
	enum class LoadThread;

	class D3D12Factory;
	class D3D12Adapter;
	class D3D12Device;
	class D3D12Fence;
	class D3D12CommandQueue;
	class D3D12CommandAllocator;
	class D3D12CommandList;
	class D3D12TransferLane;

	class SEEDCORE_API D3D12Context :public NonTransferable
	{
	public:
		D3D12Context();

		~D3D12Context();

		Bool Initialize();

		void BeginFrame();

		void EndFrame();

	public:
		Uint64 SubmitDirect();

		Uint64 SubmitCopy();

		Uint64 SubmitCompute();

	public:
		IDXGIFactory7* GetFactory()const;

		D3D12Adapter* GetAdapter()const;

		ID3D12Device* GetDevice()const;

	public:
		D3D12CommandQueue* GetDirectQueue()const;

		D3D12CommandQueue* GetCopyQueue()const;

		D3D12CommandQueue* GetComputeQueue()const;

	public:
		D3D12CommandAllocator* GetDirectAllocator()const;

		D3D12CommandAllocator* GetCopyAllocator()const;

		D3D12CommandAllocator* GetComputeAllocator()const;

	public:
		D3D12CommandList* GetDirectList()const;

		D3D12CommandList* GetCopyList()const;

		D3D12CommandList* GetComputeList()const;

	public:
		D3D12TransferLane* GetFrameTransferLane()const;

		D3D12TransferLane* GetLoadTransferLane(LoadThread thread)const;

	private:
		Bool CreateDevice();

		Bool CreateCommandAllocator();

		Bool CreateCommandList();

	private:
		ResourcePtr<D3D12Adapter> adapter_;

		ResourcePtr<D3D12Factory> factory_;

		ResourcePtr<D3D12Device> device_;

		ResourcePtr<D3D12CommandQueue> directQueue_;
		ResourcePtr<D3D12CommandQueue> copyQueue_;
		ResourcePtr<D3D12CommandQueue> computeQueue_;

		ResourcePtr<D3D12CommandAllocator> directAllocator_;
		ResourcePtr<D3D12CommandAllocator> copyAllocator_;
		ResourcePtr<D3D12CommandAllocator> computeAllocator_;

		ResourcePtr<D3D12CommandList> directList_;
		ResourcePtr<D3D12CommandList> copyList_;
		ResourcePtr<D3D12CommandList> computeList_;

		ResourcePtr<D3D12CommandAllocator> directAllocators_[2];
		ResourcePtr<D3D12CommandAllocator> copyAllocators_[2];
		ResourcePtr<D3D12CommandAllocator> computeAllocators_[2];

		Uint64 frameFenceValues_[2] = { 0, 0 };

		ResourcePtr<D3D12TransferLane> frameTransferLane_;

		ResourcePtr<D3D12TransferLane> mainLoadTransferLane_;

		ResourcePtr<D3D12TransferLane> asyncLoadTransferLane_;
	};
}