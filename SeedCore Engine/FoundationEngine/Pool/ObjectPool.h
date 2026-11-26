#pragma once

namespace SeedCore
{
#if defined(SC_POINTER_BITS)
#elif defined(__x86_64__)||defined(_M_X64)||defined(_M_AMD64)
#define SC_POINTER_BITS 48
#elif defined(__aarch64)||defined(_M_ARM64)
#define SC_POINTER_BITS 48
#elif defined(__riscv)&&__riscv_xlen == 64
#define SC_POINTER_BITS 48
#else
#define SC_POINTER_BITS (sizeof(void*) * CHAR_BIT)
#endif

	constexpr inline Size SC_CACHELINE_SIZE =
#ifdef __cpp_lib_hardware_interference_size
		std::hardware_destructive_interference_size;
#else
		64;
#endif

	struct SynchronizedPointer
	{
		using PointerType = std::uintptr_t;
		using TagType     = std::uintptr_t;

		PointerType ptr_{ 0 };

		TagType     tag_{ 0 };

		SynchronizedPointer() = default;

		SynchronizedPointer(PointerType ptr, TagType tag)noexcept;

		inline SynchronizedPointer::PointerType GetPtr()const noexcept
		{
			return ptr_;
		}

		inline SynchronizedPointer::TagType GetTag()const noexcept
		{
			return tag_;
		}
	};

	template <Int PtrBits = SC_POINTER_BITS>
	struct PackedSynchronizedPointer
	{
		using PointerType = std::uintptr_t;
		using TagType     = Uint16;

		SC_CONSTEX Int PTR_BITS = PtrBits;
		SC_CONSTEX Int TAG_BITS = 64 - PtrBits;
		SC_CONSTEX PointerType PTR_MASK = (PointerType{ 1 } << PTR_BITS) - 1;

		std::uintptr_t bits_{ 0 };

		PackedSynchronizedPointer() = default;

		PackedSynchronizedPointer(PointerType ptr, TagType tag)noexcept :bits_((ptr& PTR_MASK) | (static_cast<std::uintptr_t>(tag) << PTR_BITS))
		{
			/// No Code
		}

		inline PointerType GetPtr()const noexcept
		{
			return bits_ & PTR_MASK;
		}

		inline TagType GetTag()const noexcept
		{
			return static_cast<TagType>(bits_ >> PTR_BITS);
		}
	};

	template<typename T>
	struct ObjectBlock
	{
		Uint16 poolID_;

		std::atomic<ObjectBlock*> nextFree_ = nullptr;

		alignas(T)std::byte storage_[sizeof(T)];

		T* object()noexcept
		{
			return std::launder(reinterpret_cast<T*>(storage_));
		}

		const T* object()const noexcept
		{
			return std::launder(reinterpret_cast<const T*>(storage_));
		}

		static ObjectBlock<T>* from(T* obj)noexcept
		{
			return reinterpret_cast<ObjectBlock*>(reinterpret_cast<Char*>(obj) - offsetof(ObjectBlock, storage_));
		}
	};

	template<typename T, typename H = SynchronizedPointer, Size LogSize = 5>
	class ObjectPool
	{
	private:
		using Block = ObjectBlock<T>;

		SC_CONSTEX Size NumberPools = 1u << LogSize;

		struct alignas(SC_CACHELINE_SIZE) Shard
		{
			std::atomic<H> freeHead_{ H{} };

			alignas(SC_CACHELINE_SIZE) std::pmr::synchronized_pool_resource backing_
			{
				std::pmr::pool_options
				{
					.max_blocks_per_chunk = 1024,
					.largest_required_pool_block = sizeof(Block)
				}
			};

			void PushFree(Block* block)noexcept
			{
				H current = freeHead_.load(std::memory_order_relaxed);
				H next;
				do
				{
					block->nextFree_.store(reinterpret_cast<Block*>(current.GetPtr()), std::memory_order_relaxed);
					next = H(reinterpret_cast<typename H::PointerType>(block), static_cast<typename H::TagType>(current.GetTag() + 1));
				} while (!freeHead_.compare_exchange_weak(current, next, std::memory_order_release, std::memory_order_relaxed));
			}

			Block* PopFree()noexcept
			{
				H current = freeHead_.load(std::memory_order_acquire);
				while (current.GetPtr())
				{
					auto* ptr = reinterpret_cast<Block*>(current.GetPtr());
					Block* nextBlock = ptr->nextFree_.load(std::memory_order_relaxed);
					H next(reinterpret_cast<typename H::PointerType>(nextBlock), static_cast<typename H::TagType>(current.GetTag() + 1));
					if (freeHead_.compare_exchange_weak(current, next, std::memory_order_acquire, std::memory_order_acquire))
					{
						return ptr;
					}
				}
				return nullptr;
			}

			Block* AllocateBacking()
			{
				return static_cast<Block*>(backing_.allocate(sizeof(Block), alignof(Block)));
			}

			void DeallocateBacking(Block* block)
			{
				backing_.deallocate(block, sizeof(Block), alignof(Block));
			}
		};

		StaticArray<Shard, NumberPools> shards_;

		static Size Next()noexcept
		{
			thread_local Size counter = std::hash<std::thread::id>{}(std::this_thread::get_id());
			return counter++ & (NumberPools - 1);
		}

	public:
		ObjectPool() = default;

		ObjectPool(const ObjectPool&) = delete;

		ObjectPool& operator=(const ObjectPool&) = delete;

		~ObjectPool() = default;

		template<typename... Args>
		NODISCARD T* Create(Args&&... args)
		{
			auto sid = Next();
			auto& shard = shards_[sid];

			Block* block = shard.PopFree();
			if (!block)
			{
				block = shard.AllocateBacking();
			}

			block->poolID_ = static_cast<Uint16>(sid);
			return std::construct_at(block->object(), std::forward<Args>(args)...);
		}

		void Recycle(T* obj)
		{
			if (!obj)
			{
				return;
			}
			auto* block = Block::from(obj);
			std::destroy_at(block->object());
			shards_[block->poolID_].PushFree(block);
		}

		void Release()
		{
			for (auto& shard : shards_)
			{
				shard.backing_.release();
				shard.freeHead_.store(H{}, std::memory_order_relaxed);
			}
		}
	};
}