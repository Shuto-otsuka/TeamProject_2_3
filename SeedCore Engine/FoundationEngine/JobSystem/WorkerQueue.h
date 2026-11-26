#pragma once
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	template<typename T>
	concept Nullable = std::is_pointer_v<T>;

	template<typename T>
	concept Optionalable = !std::is_pointer_v<T>;

	template <typename T>
	constexpr auto WorkerQueueEmptyValue()
	{
		if constexpr (Nullable<T>)
		{
			return T{ nullptr };
		}
		else if constexpr (Optionalable<T>)
		{
			return std::optional<T>{std::nullopt};
		}
	}

	inline constexpr Size SC_DEFAULT_UNBOUNDED_QUEUE_LOG_SIZE = 10;

	inline constexpr Size SC_DEFAULT_BOUNDED_QUEUE_LOG_SIZE = 8;

	template <typename T>
	class UnboundedWorkerQueue
	{
	private:
		struct Array
		{
			Size capacity_;

			Size mask_;

			std::atomic<T>* slots_;

			explicit Array(Size capacity) :capacity_(capacity), mask_(capacity - 1), slots_(new std::atomic<T>[capacity_])
			{
				/// No Code
			}

			~Array()
			{
				delete[] slots_;
			}

			Size capacity()const noexcept
			{
				return capacity_;
			}

			void push(Int64 index, T item)noexcept
			{
				slots_[index & mask_].store(item, std::memory_order_relaxed);
			}

			T pop(Int64 index)noexcept
			{
				return slots_[index & mask_].load(std::memory_order_relaxed);
			}

			Array* resize(Int64 bottom, Int64 top)
			{
				Array* ptr = new Array(2 * capacity_);
				for (Int64 index = top;index != bottom;index++)
				{
					ptr->push(index, pop(index));
				}
				return ptr;
			}

			Array* resize(Int64 bottom, Int64 top, Int64 n)
			{
				Array* ptr = new Array(std::bit_ceil(capacity_ + n));
				for (Int64 index = top;index != bottom;++index)
				{
					ptr->push(index, pop(index));
				}
				return ptr;
			}
		};

		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> top_;

		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> bottom_;

		Int64 cacheTop_ = 0;

		alignas(SC_CACHELINE_SIZE) std::atomic<Array*> array_;

		DynamicArray<Array*> garbage_;

	public:
		using ValueType = std::conditional_t<std::is_pointer_v<T>, T, std::optional<T>>;

		explicit UnboundedWorkerQueue(Int64 logSize = SC_DEFAULT_UNBOUNDED_QUEUE_LOG_SIZE)
		{
			top_.store(0, std::memory_order_relaxed);
			bottom_.store(0, std::memory_order_relaxed);
			array_.store(new Array{ (Size{ 1 } << logSize) }, std::memory_order_relaxed);

			garbage_.reserve(32);
		}

		~UnboundedWorkerQueue()
		{
			for (auto garbage : garbage_)
			{
				delete garbage;
			}
			delete array_.load();
		}

		Bool empty()const noexcept
		{
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return (bottom <= top);
		}

		Size size()const noexcept
		{
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return static_cast<Size>(bottom >= top ? bottom - top : 0);
		}

		Size capacity()const noexcept
		{
			return array_.load(std::memory_order_relaxed)->capacity();
		}

		void push(T item)
		{
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Array* array = array_.load(std::memory_order_relaxed);

			if (array->capacity() < static_cast<Size>(bottom - cacheTop_ + 1)) [[unlikely]]
			{
				cacheTop_ = top_.load(std::memory_order_acquire);
				if (array->capacity() < static_cast<Size>(bottom - cacheTop_ + 1)) [[unlikely]]
				{
					array = resize_array(array, bottom, cacheTop_);
				}
			}

			array->push(bottom, item);

			std::atomic_thread_fence(std::memory_order_release);

			bottom_.store(bottom + 1, std::memory_order_release);
		}

		template <typename I>
		void bulk_push(I& first, Size n)
		{
			if (n == 0)
			{
				return;
			}

			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Array* array = array_.load(std::memory_order_relaxed);

			if ((bottom - cacheTop_ + n) > array->capacity()) [[unlikely]]
			{
				cacheTop_ = top_.load(std::memory_order_acquire);
				if ((bottom - cacheTop_ + n) > array->capacity()) [[unlikely]]
				{
					array = resize_array(array, bottom, cacheTop_, n);
				}
			}

			for (Size index = 0;index < n;++index)
			{
				array->push(bottom++, *first++);
			}

			std::atomic_thread_fence(std::memory_order_release);

			bottom_.store(bottom, std::memory_order_release);
		}

		ValueType pop()
		{
			Int64 bottom = bottom_.load(std::memory_order_relaxed) - 1;
			Array* array = array_.load(std::memory_order_relaxed);
			bottom_.store(bottom, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 top = top_.load(std::memory_order_relaxed);

			auto item = empty_value();

			if (top <= bottom)
			{
				item = array->pop(bottom);
				if (top == bottom)
				{
					if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
					{
						item = empty_value();
					}
					bottom_.store(bottom + 1, std::memory_order_relaxed);
				}
			}
			else
			{
				bottom_.store(bottom + 1, std::memory_order_relaxed);
			}

			return item;
		}

		ValueType steal()
		{
			Int64 top = top_.load(std::memory_order_acquire);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 bottom = bottom_.load(std::memory_order_acquire);

			auto item = empty_value();

			if (top < bottom)
			{
				Array* array = array_.load(std::memory_order_consume);
				item = array->pop(top);
				if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
				{
					return empty_value();
				}
			}

			return item;
		}

		SC_CONSTEX auto empty_value()
		{
			return WorkerQueueEmptyValue<T>();
		}

	private:
		Array* resize_array(Array* array, Int64 bottom, Int64 top)
		{
			Array* temporary = array->resize(bottom, top);
			garbage_.push_back(array);
			array_.store(temporary, std::memory_order_release);
			return temporary;
		}

		Array* resize_array(Array* array, Int64 bottom, Int64 top, Size n)
		{
			Array* temporary = array->resize(bottom, top, n);
			garbage_.push_back(array);
			array_.store(temporary, std::memory_order_release);
			return temporary;
		}
	};

	template <typename T, Size logSize = SC_DEFAULT_BOUNDED_QUEUE_LOG_SIZE>
	class BoundedWorkerQueue
	{
	private:
		SC_CONSTEX Size bufferSize = Size{ 1 } << logSize;

		SC_CONSTEX Size bufferMask = (bufferSize - 1);

		static_assert((bufferSize >= 2) && ((bufferSize& (bufferSize - 1)) == 0));

		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> top_{ 0 };

		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> bottom_{ 0 };

		alignas(SC_CACHELINE_SIZE) std::atomic<T> buffer_[bufferSize];

	public:
		using ValueType = std::conditional_t<std::is_pointer_v<T>, T, std::optional<T>>;

		BoundedWorkerQueue() = default;

		~BoundedWorkerQueue() = default;

		Bool empty()const noexcept
		{
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return bottom <= top;
		}

		Size size()const noexcept
		{
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return static_cast<Size>(bottom >= top ? bottom - top : 0);
		}

		constexpr Size capacity()const
		{
			return bufferSize;
		}

		template <typename O>
		Bool try_push(O&& item)
		{
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Int64 top = top_.load(std::memory_order_acquire);

			if (static_cast<Size>(bottom - top + 1) > bufferSize) [[unlikely]]
			{
				return false;
			}

			buffer_[bottom & bufferMask].store(std::forward<O>(item), std::memory_order_relaxed);

			std::atomic_thread_fence(std::memory_order_release);

			bottom_.store(bottom + 1, std::memory_order_release);

			return true;
		}

		template <typename I>
		Size try_bulk_push(I& first, Size n)
		{
			if (n == 0)
			{
				return 0;
			}

			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Int64 top = top_.load(std::memory_order_acquire);

			Size remaining = bufferSize - (bottom - top);
			Size number = Min(n, remaining);

			if (n > 0)
			{
				for (Size index = 0;index < number;index++)
				{
					buffer_[bottom++ & bufferMask].store(*first++, std::memory_order_relaxed);
				}
				std::atomic_thread_fence(std::memory_order_release);

				bottom_.store(bottom, std::memory_order_release);
			}

			return number;
		}

		ValueType pop()
		{
			Int64 bottom = bottom_.load(std::memory_order_relaxed) - 1;
			bottom_.store(bottom, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 top = top_.load(std::memory_order_relaxed);

			auto item = empty_value();

			if (top <= bottom)
			{
				item = buffer_[bottom & bufferMask].load(std::memory_order_relaxed);
				if (top == bottom)
				{
					if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
					{
						item = empty_value();
					}
					bottom_.store(bottom + 1, std::memory_order_relaxed);
				}
			}
			else
			{
				bottom_.store(bottom + 1, std::memory_order_relaxed);
			}

			return item;
		}

		ValueType steal()
		{
			Int64 top = top_.load(std::memory_order_acquire);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 bottom = bottom_.load(std::memory_order_acquire);

			auto item = empty_value();

			if (top < bottom)
			{
				item = buffer_[top & bufferMask].load(std::memory_order_relaxed);
				if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
				{
					return empty_value();
				}
			}

			return item;
		}

		SC_CONSTEX auto empty_value()
		{
			return WorkerQueueEmptyValue<T>();
		}
	};
}