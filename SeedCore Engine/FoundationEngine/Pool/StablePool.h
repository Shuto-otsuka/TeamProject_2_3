#pragma once
#include <FoundationEngine/Memory/ChunkAllocator.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	template <typename T>
	concept Poolable = std::is_nothrow_destructible_v<T> && std::is_nothrow_move_constructible_v<T>;

	template <Poolable T>
	class StablePool
	{
	public:
		StablePool() = default;

		StablePool(const StablePool&) = delete;
		StablePool& operator=(const StablePool&) = delete;

		~StablePool()
		{
			for (auto& slot : slots_)
			{
				if (slot.alive_ && slot.obj_)
				{
					slot.obj_->~T();
					slot.obj_ = nullptr;
					slot.alive_ = false;
				}
			}
		}

		Handle<T> Create()
		{
			Uint32 index;

			if (!freelist_.empty())
			{
				index = freelist_.back();
				freelist_.pop_back();
			}
			else
			{
				index = static_cast<Uint32>(slots_.size());
				slots_.emplace_back();
			}

			Slot& slot = slots_[index];

			T* obj = allocator_.allocate();
			new(obj)T();

			slot.obj_ = obj;
			slot.alive_ = true;

			return Handle<T>{index, slot.generation_};
		}

		T* Get(const Handle<T>& handle)
		{
			if (!handle.exists())
			{
				return nullptr;
			}

			if (handle.index_ >= slots_.size())
			{
				return nullptr;
			}

			Slot& slot = slots_[handle.index_];

			if (!slot.alive_)
			{
				return nullptr;
			}

			if (slot.generation_ != handle.generation_)
			{
				return nullptr;
			}

			return slot.obj_;
		}

		const T* Get(const Handle<T>& handle)const
		{
			if (!handle.exists())
			{
				return nullptr;
			}

			if (handle.index_ >= slots_.size())
			{
				return nullptr;
			}

			const Slot& slot = slots_[handle.index_];

			if (!slot.alive_)
			{
				return nullptr;
			}

			if (slot.generation_ != handle.generation_)
			{
				return nullptr;
			}

			return slot.obj_;
		}

		void Destroy(const Handle<T>& handle)
		{
			if (!handle.exists())
			{
				return;
			}

			if (handle.index_ >= slots_.size())
			{
				return;
			}

			Slot& slot = slots_[handle.index_];

			if (!slot.alive_)
			{
				return;
			}

			if (slot.generation_ != handle.generation_)
			{
				return;
			}

			slot.obj_->~T();

			slot.obj_ = nullptr;
			slot.alive_ = false;
			slot.generation_++;

			freelist_.push_back(static_cast<Uint32>(handle.index_));
		}

	private:
		struct Slot
		{
			T* obj_ = nullptr;
			Uint64 generation_ = 1;
			Bool alive_ = false;
		};
		DynamicArray<Slot> slots_;
		DynamicArray<Uint32> freelist_;

		ChunkAllocator<T> allocator_;
	};
}