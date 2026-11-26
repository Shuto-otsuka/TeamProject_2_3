#pragma once

namespace SeedCore
{
	template<typename T, Size ChunkSize = 256>
	class ChunkAllocator
	{
	public:
		struct Chunk
		{
			alignas(T) std::byte data_[sizeof(T) * ChunkSize];

			Uint32 used_ = 0;
		};

		T* allocate()
		{
			if (chunks_.empty() || chunks_.back()->used_ == ChunkSize)
			{
				chunks_.push_back(MakePtr<Chunk>());
			}

			Chunk* chunk = chunks_.back().get();
			return reinterpret_cast<T*>(&chunk->data_[chunk->used_++ * sizeof(T)]);
		}

	private:
		DynamicArray<ResourcePtr<Chunk>> chunks_;
	};
}