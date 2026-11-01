#pragma once

namespace SeedCore
{
	class D3D12CommandQueue;
	class StagingBuffer;

	class SEEDCORE_API D3D12TransferStream :public NonTransferable
	{
	public: 
		explicit D3D12TransferStream(Uint64 pageSize);

		~D3D12TransferStream();

		void Upload(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination, Uint32 subresource, std::span<const std::byte> data);

		NODISCARD std::span<const std::byte> Readback(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* source, Uint32 subresource);

		NODISCARD Uint64 Submit(D3D12CommandQueue& cmdQueue, ID3D12GraphicsCommandList* cmdList);

		void Retire(D3D12CommandQueue& cmdQueue);

	private:
		struct StagingLane;

		NODISCARD std::optional<Uint64> Reserve(ID3D12Device* device, StagingLane& lane, Uint64 size, Uint64 alignment);

	private:
		struct Staging
		{
			ResourcePtr<StagingBuffer> buffer_;
			Uint64 fenceValue_ = 0;
		};

		struct StagingLane
		{
			D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_UPLOAD;
			DynamicArray<Staging> recording_;
			DynamicArray<Staging> pending_;
			DynamicArray<Staging> free_;
		};

		StagingLane upload_;

		StagingLane readback_;

		Uint64 pageSize_ = 0;
	};
}