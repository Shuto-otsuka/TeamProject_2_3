#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandAllocator.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>

namespace SeedCore
{
	D3D12TransferLane::D3D12TransferLane(ID3D12Device* device, const D3D12CommandQueue& cmdQueue, Uint64 pageSize) :stream_(pageSize)
	{
		Allocator allocator;
		allocator.allocator_ = MakePtr<D3D12CommandAllocator>();
		allocator.allocator_->Create(device, cmdQueue);
		allocators_.push_back(std::move(allocator));

		cmdList_ = MakePtr<D3D12CommandList>();
		cmdList_->Create(device, cmdQueue, *allocators_.front().allocator_);
		cmdList_->Close();
	}

	D3D12TransferLane::~D3D12TransferLane() = default;

	void D3D12TransferLane::Begin(ID3D12Device* device, D3D12CommandQueue& cmdQueue)
	{
		stream_.Retire(cmdQueue);

		Uint64 completedFenceValue = cmdQueue.GetFence()->GetCompletedValue();
		auto found = std::ranges::find_if(allocators_, [completedFenceValue](const Allocator& allocator) { return allocator.fenceValue_ <= completedFenceValue; });
		if (found == allocators_.end())
		{
			Allocator allocator;
			allocator.allocator_ = MakePtr<D3D12CommandAllocator>();
			allocator.allocator_->Create(device, cmdQueue);
			allocators_.push_back(std::move(allocator));
			found = std::prev(allocators_.end());
		}

		recordingIndex_ = static_cast<Size>(std::distance(allocators_.begin(), found));
		found->fenceValue_ = std::numeric_limits<Uint64>::max();
		found->allocator_->Reset();
		cmdList_->Reset(found->allocator_->Get());
	}

	Uint64 D3D12TransferLane::Submit(D3D12CommandQueue& cmdQueue)
	{
		Uint64 fenceValue = stream_.Submit(cmdQueue, cmdList_->Get());
		allocators_[recordingIndex_].fenceValue_ = fenceValue;
		return fenceValue;
	}

	ID3D12GraphicsCommandList* D3D12TransferLane::GetCommandList()const
	{
		return cmdList_->Get();
	}

	D3D12TransferStream& D3D12TransferLane::GetStream()
	{
		return stream_;
	}
}