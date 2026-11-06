#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	StagingBuffer::StagingBuffer(ID3D12Device* device, Uint64 size, D3D12_HEAP_TYPE type) :Buffer(device, size, type, D3D12_RESOURCE_FLAG_NONE, type == D3D12_HEAP_TYPE_READBACK ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ)
	{
		if (type != D3D12_HEAP_TYPE_UPLOAD && type != D3D12_HEAP_TYPE_READBACK)
		{
			SC_LOG_WARNING("StagingBuffer には UPLOAD か READBACK のヒープしか使えません。");
			return;
		}

		if (!resource_)
		{
			return;
		}

		void* mappedAddress = nullptr;
		D3D12_RANGE readRange{ 0,type == D3D12_HEAP_TYPE_READBACK ? size : 0 };
		HRESULT hr = resource_->Map(0, &readRange, &mappedAddress);
		SC_HR_CHECK(hr, "ステージングバッファのMapに失敗しました");
		if (FAILED(hr))
		{
			return;
		}

		mappedPtr_ = static_cast<std::byte*>(mappedAddress);
		size_ = size;	
	}

	NODISCARD std::optional<Uint64> StagingBuffer::Allocate(Uint64 size, Uint64 alignment)
	{
		Uint64 offset = (used_ + alignment - 1) / alignment * alignment;
		if (offset > size_ || size > size_ - offset)
		{
			return std::nullopt;
		}

		used_ = offset + size;
		return offset;
	}

	void StagingBuffer::Reset()
	{
		used_ = 0;
	}

	void StagingBuffer::Write(Uint64 offset, std::span<const std::byte> data)
	{
		memcpy(mappedPtr_ + offset, data.data(), data.size());
	}

	NODISCARD std::span<const std::byte> StagingBuffer::Read(Uint64 offset, Uint64 size)const
	{
		return std::span<const std::byte>(mappedPtr_ + offset, size);
	}

	NODISCARD Uint64 StagingBuffer::Size()const
	{
		return size_;
	}

	Bool StagingBuffer::ViewImplementation()const
	{
		return true;
	}
}