#pragma once
#include <GraphicsEngine/D3D12/Buffer/Buffer.h>

namespace SeedCore
{
	class StagingBuffer :public Buffer<StagingBuffer>
	{
	public:
		StagingBuffer(ID3D12Device* device, Uint64 size, D3D12_HEAP_TYPE type);

		NODISCARD std::optional<Uint64> Allocate(Uint64 size, Uint64 alignment);

		void Reset();

		void Write(Uint64 offset, std::span<const std::byte> data);

		NODISCARD std::span<const std::byte> Read(Uint64 offset, Uint64 size)const;

		NODISCARD Uint64 Size()const;

		Bool ViewImplementation()const;

	private:
		std::byte* mappedPtr_ = nullptr;

		Uint64 size_ = 0;

		Uint64 used_ = 0;
	};
}