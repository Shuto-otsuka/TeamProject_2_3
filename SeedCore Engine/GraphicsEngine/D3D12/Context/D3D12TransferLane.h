#pragma once
#include <GraphicsEngine/D3D12/Context/D3D12TransferStream.h>

namespace SeedCore
{
	class D3D12CommandQueue;
	class D3D12CommandAllocator;
	class D3D12CommandList;

	class SEEDCORE_API D3D12TransferLane :public NonTransferable
	{
	public:
		D3D12TransferLane(ID3D12Device* device, const D3D12CommandQueue& cmdQueue, Uint64 pageSize);

		~D3D12TransferLane();

		void Begin(ID3D12Device* device, D3D12CommandQueue& cmdQueue);

		NODISCARD Uint64 Submit(D3D12CommandQueue& cmdQueue);

		NODISCARD ID3D12GraphicsCommandList* GetCommandList()const;

		NODISCARD D3D12TransferStream& GetStream();

	private:
		struct Allocator
		{
			ResourcePtr<D3D12CommandAllocator> allocator_;

			Uint64 fenceValue_ = 0;
		};

		DynamicArray<Allocator> allocators_;

		ResourcePtr<D3D12CommandList> cmdList_;

		D3D12TransferStream stream_;

		Size recordingIndex_ = 0;
	};
}