#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	RenderUpload::RenderUpload() = default;

	RenderUpload::~RenderUpload() = default;

	void RenderUpload::Initialize(RenderDevice& device, Uint64 capacity)
	{
		for (ResourcePtr<StagingBuffer>& buffer : buffers_)
		{
			buffer = MakePtr<StagingBuffer>(device.Context().GetDevice(), capacity, D3D12_HEAP_TYPE_UPLOAD);
		}
	}

	void RenderUpload::Reset(Uint32 frame)
	{
		current_ = frame % FrameRing::frameCount;
		buffers_[current_]->Reset();
		entries_.clear();
	}

	void RenderUpload::Write(const RenderResource& resource, RenderViewKind view, std::span<const std::byte> data, Uint64 alignment)
	{
		std::optional<Uint64> offset = buffers_[current_]->Allocate((data.size() + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, alignment);
		if (!offset)
		{
			SC_LOG_ERROR("RenderUpload: 容量（{} バイト）が足りません（{} に {} バイト）", buffers_[current_]->Size(), resource.Name(), data.size());
			return;
		}
		buffers_[current_]->Write(*offset, data);
		entries_.push_back(RenderUploadEntry{ String(resource.Name()), view, *offset, data.size() });
	}

	const RenderUploadEntry* RenderUpload::Find(const String& resourceName, RenderViewKind view)const
	{
		for (const RenderUploadEntry& entry : entries_)
		{
			if (entry.resourceName_ == resourceName && entry.view_ == view)
			{
				return &entry;
			}
		}
		return nullptr;
	}

	StagingBuffer& RenderUpload::Buffer()const
	{
		return *buffers_[current_];
	}
}