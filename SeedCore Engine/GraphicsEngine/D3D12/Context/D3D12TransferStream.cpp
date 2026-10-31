#include <GraphicsEngine/D3D12/Context/D3D12TransferStream.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	D3D12TransferStream::D3D12TransferStream(Uint64 pageSize) :pageSize_(pageSize)
	{
		upload_.heapType_ = D3D12_HEAP_TYPE_UPLOAD;
		readback_.heapType_ = D3D12_HEAP_TYPE_READBACK;
	}

	D3D12TransferStream::~D3D12TransferStream() = default;

	void D3D12TransferStream::Upload(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination, Uint32 subresource, std::span<const std::byte> data)
	{
		D3D12_RESOURCE_DESC desc = destination->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		Uint32 rowCount = 0;
		Uint64 rowSize = 0;
		Uint64 totalSize = 0;
		device->GetCopyableFootprints(&desc, subresource, 1, 0, &footprint, &rowCount, &rowSize, &totalSize);

		Uint64 totalRowCount = static_cast<Uint64>(rowCount) * footprint.Footprint.Depth;
		if (data.size() < rowSize * totalRowCount)
		{
			SC_LOG_WARNING("転送するデータが足りません （{} < {}）", data.size(), rowSize * totalRowCount);
			return;
		}

		std::optional<Uint64> reserved = Reserve(device, upload_, totalSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
		if (!reserved)
		{
			return;
		}

		Uint64 offset = *reserved;
		StagingBuffer& staging = *upload_.recording_.back().buffer_;
		for (Uint64 rowIndex = 0; rowIndex < totalRowCount; rowIndex++)
		{
			staging.Write(offset + rowIndex * footprint.Footprint.RowPitch, data.subspan(rowIndex * rowSize, rowSize));
		}

		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			cmdList->CopyBufferRegion(destination, 0, staging.Resource(), offset, desc.Width);
			return;
		}

		footprint.Offset = offset;

		D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
		destinationLocation.pResource = destination;
		destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destinationLocation.SubresourceIndex = subresource;

		D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
		sourceLocation.pResource = staging.Resource();
		sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		sourceLocation.PlacedFootprint = footprint;

		cmdList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
	}

	std::span<const std::byte> D3D12TransferStream::Readback(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* source, Uint32 subresource)
	{
		D3D12_RESOURCE_DESC desc = source->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		Uint64 totalSize = 0;
		device->GetCopyableFootprints(&desc, subresource, 1, 0, &footprint, nullptr, nullptr, &totalSize);

		std::optional<Uint64> reserved = Reserve(device, readback_, totalSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
		if (!reserved)
		{
			return {};
		}

		Uint64 offset = *reserved;
		StagingBuffer& staging = *readback_.recording_.back().buffer_;
		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			cmdList->CopyBufferRegion(staging.Resource(), offset, source, 0, desc.Width);
			return staging.Read(offset, totalSize);
		}

		footprint.Offset = offset;

		D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
		destinationLocation.pResource = staging.Resource();
		destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destinationLocation.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
		sourceLocation.pResource = source;
		sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		sourceLocation.SubresourceIndex = subresource;

		cmdList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
		return staging.Read(offset, totalSize);
	}

	Uint64 D3D12TransferStream::Submit(D3D12CommandQueue& cmdQueue, ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->Close();
		cmdQueue.Execute(cmdList);
		Uint64 fenceValue = cmdQueue.Signal();

		for (StagingLane* lane : { &upload_, &readback_ })
		{
			for (Staging& staging : lane->recording_)
			{
				staging.fenceValue_ = fenceValue;
				lane->pending_.push_back(std::move(staging));
			}
			lane->recording_.clear();
		}

		return fenceValue;
	}

	void D3D12TransferStream::Retire(D3D12CommandQueue& cmdQueue)
	{
		Uint64 completedFenceValue = cmdQueue.GetFence()->GetCompletedValue();

		for (StagingLane* lane : { &upload_,&readback_ })
		{
			for (Staging& staging : lane->pending_)
			{
				if (staging.fenceValue_ > completedFenceValue)
				{
					continue;
				}

				staging.buffer_->Reset();
				lane->free_.push_back(std::move(staging));
			}

			SeedCore::erase_if(lane->pending_, [](const Staging& staging) {return !staging.buffer_; });
		}
	}

	std::optional<Uint64> D3D12TransferStream::Reserve(ID3D12Device* device, StagingLane& lane, Uint64 size, Uint64 alignment)
	{
		if (!lane.recording_.empty())
		{
			std::optional<Uint64> allocated = lane.recording_.back().buffer_->Allocate(size,alignment);
			if (allocated)
			{
				return allocated;
			}
		}

		Uint64 requiredSize = size + alignment;
		auto found = std::ranges::find_if(lane.free_, [requiredSize](const Staging& staging) { return staging.buffer_->Size() >= requiredSize; });
		if (found != lane.free_.end())
		{
			lane.recording_.push_back(std::move(*found));
			lane.free_.erase(found);
		}
		else
		{
			Staging staging;
			staging.buffer_ = MakePtr<StagingBuffer>(device, Max(pageSize_, requiredSize), lane.heapType_);
			lane.recording_.push_back(std::move(staging));
		}

		return lane.recording_.back().buffer_->Allocate(size, alignment);
	}
}