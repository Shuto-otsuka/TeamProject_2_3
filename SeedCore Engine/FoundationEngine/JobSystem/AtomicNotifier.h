#pragma once
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/Log/Assert.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	class AtomicNotifier :public NonTransferable
	{
	private:
		friend class JobExecutor;

	public:
		struct Waiter
		{
			alignas(2 * SC_CACHELINE_SIZE) Uint32 epoch_;
		};

		AtomicNotifier(Size n)noexcept :state_(0), waiters_(n)
		{
			/// No Code
		}

		~AtomicNotifier()
		{
			SC_ASSERT((state_.load(std::memory_order_relaxed) & WAITER_MASK) == 0, "通知処理の開始時に待機中のスレッドが残っています（State: {:#x}）", state_.load(std::memory_order_relaxed));
		}

		Size count()const noexcept
		{
			return state_.load(std::memory_order_relaxed) & WAITER_MASK;
		}

		void prepare_wait(Size waiter)noexcept
		{
			auto previous = state_.fetch_add(WAITER_INC, std::memory_order_relaxed);
			waiters_[waiter].epoch_ = (previous >> EPOCH_SHIFT);

			std::atomic_thread_fence(std::memory_order_seq_cst);
		}

		void commit_wait(Size waiter)noexcept
		{
			Uint64 previous = state_.load(std::memory_order_relaxed);
			while ((previous >> EPOCH_SHIFT) == waiters_[waiter].epoch_)
			{
				state_.wait(previous, std::memory_order_relaxed);
				previous = state_.load(std::memory_order_relaxed);
			}

			state_.fetch_sub(WAITER_INC, std::memory_order_relaxed);
		}

		void cancel_wait(Size )noexcept
		{
			state_.fetch_sub(WAITER_INC, std::memory_order_relaxed);
		}

		void notify_one()noexcept
		{
			std::atomic_thread_fence(std::memory_order_seq_cst);

			for (Uint64 state = state_.load(std::memory_order_relaxed);state & WAITER_MASK;)
			{
				if (state_.compare_exchange_weak(state, state + EPOCH_INC, std::memory_order_relaxed))
				{
					state_.notify_one();
					break;
				}
			}
		}

		void notify_all()noexcept
		{
			std::atomic_thread_fence(std::memory_order_seq_cst);

			for (Uint64 state = state_.load(std::memory_order_relaxed);state & WAITER_MASK;)
			{
				if (state_.compare_exchange_weak(state, state + EPOCH_INC, std::memory_order_relaxed))
				{
					state_.notify_all();
					break;
				}
			}
		}

		void notify_count(Size n)noexcept
		{
			if (n >= waiters_.size())
			{
				notify_all();
			}
			else
			{
				for (Size waiterIndex = 0;waiterIndex < n;++waiterIndex)
				{
					notify_one();
				}
			}
		}

		Size size()const noexcept
		{
			return waiters_.size();
		}

	private:
		SC_STATIC_ASSERT(sizeof(Int) == 4, "Int型は4バイトでなければなりません");
		SC_STATIC_ASSERT(sizeof(Uint32) == 4, "Uint32型は4バイトでなければなりません");
		SC_STATIC_ASSERT(sizeof(Uint64) == 8, "Uint64型は8バイトでなければなりません");
		SC_STATIC_ASSERT(sizeof(std::atomic<Uint64>) == 8, "std::atomic<Uint64>型は8バイトでなければなりません");

		std::atomic<Uint64> state_;

		DynamicArray<Waiter> waiters_;

		SC_CONSTEX Uint64 EPOCH_SHIFT = 32;

		SC_CONSTEX Uint64 EPOCH_INC = Uint64(1) << EPOCH_SHIFT;

		SC_CONSTEX Uint64 WAITER_INC = 1;

		SC_CONSTEX Uint64 WAITER_MASK = EPOCH_INC - 1;
	};
}
