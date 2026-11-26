#pragma once
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/Log/Exeption.h>
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	class NonblockingNotifier
	{
	private:
		friend class JobExecutor;

		struct Waiter
		{
			alignas(SC_CACHELINE_SIZE) std::atomic<Waiter*> next_;

			Uint64 epoch_;

			enum : Unsigned
			{
				NonSignaled = 0,

				Waiting,

				Signaled,
			};

			std::atomic<Unsigned> state_{ 0 };
		};

	public:
		static const Uint64 STACK_BITS = 16;

		static const Uint64 STACK_MASK = (1ull << STACK_BITS) - 1;

		static const Uint64 PREWAITER_BITS = 16;

		static const Uint64 PREWAITER_SHIFT = 16;

		static const Uint64 PREWAITER_MASK = ((1ull << PREWAITER_BITS) - 1) << PREWAITER_SHIFT;

		static const Uint64 PREWAITER_INC = 1ull << PREWAITER_BITS;

		static const Uint64 EPOCH_BITS = 32;

		static const Uint64 EPOCH_SHIFT = 32;

		static const Uint64 EPOCH_MASK = ((1ull << EPOCH_BITS) - 1) << EPOCH_SHIFT;

		static const Uint64 EPOCH_INC = 1ull << EPOCH_SHIFT;

		explicit NonblockingNotifier(Size n) :state_(STACK_MASK), waiters_(n)
		{
			if (waiters_.size() >= ((1 << PREWAITER_BITS) - 1))
			{
				SC_THROW("NonblockingNotifier で設定可能な待機スレッド数は最大 {} 個までです。", (1 << PREWAITER_BITS) - 1);
			}
		}

		~NonblockingNotifier()
		{
			SC_ASSERT((state_.load() & (STACK_MASK | PREWAITER_MASK)) == STACK_MASK);
		}

		Size count()const
		{
			Size n = 0;
			for (const auto& resource : waiters_)
			{
				if (resource.state_.load(std::memory_order_relaxed) == Waiter::Waiting)
				{
					n++;
				}
			}
			return n;
		}

		Size capacity()const
		{
			return 1 << STACK_BITS;
		}

		void prepare_wait(Size wid)
		{
			waiters_[wid].epoch_ = state_.fetch_add(PREWAITER_INC, std::memory_order_relaxed);

			std::atomic_thread_fence(std::memory_order_seq_cst);
		}

		void commit_wait(Size wid)
		{
			auto waiter = &waiters_[wid];
			waiter->state_.store(Waiter::NonSignaled, std::memory_order_relaxed);

			Uint64 epoch = (waiter->epoch_ & EPOCH_MASK) + (((waiter->epoch_ & PREWAITER_MASK) >> PREWAITER_SHIFT) << EPOCH_SHIFT);
			Uint64 state = state_.load(std::memory_order_seq_cst);
			for (;;)
			{
				if (Int64((state & EPOCH_MASK) - epoch) < 0)
				{
					std::this_thread::yield();
					state = state_.load(std::memory_order_seq_cst);
					continue;
				}

				if (Int64((state & EPOCH_MASK) - epoch) > 0)
				{
					return;
				}

				Uint64 newstate = state - PREWAITER_INC + EPOCH_INC;
				newstate = (newstate & ~STACK_MASK) | wid;

				if ((state & STACK_MASK) == STACK_MASK)
				{
					waiter->next_.store(nullptr, std::memory_order_relaxed);
				}
				else
				{
					waiter->next_.store(&waiters_[state & STACK_MASK], std::memory_order_relaxed);
				}

				if (state_.compare_exchange_weak(state, newstate, std::memory_order_release))
				{
					break;
				}
			}

			park(waiter);
		}

		void cancel_wait(Size wid)
		{
			Uint64 epoch = (waiters_[wid].epoch_ & EPOCH_MASK) + (((waiters_[wid].epoch_ & PREWAITER_MASK) >> PREWAITER_SHIFT) << EPOCH_SHIFT);
			Uint64 state = state_.load(std::memory_order_relaxed);
			for (;;)
			{
				if (Int64((state & EPOCH_MASK) - epoch) < 0)
				{
					std::this_thread::yield();
					state = state_.load(std::memory_order_relaxed);
					continue;
				}

				if (Int64((state & EPOCH_MASK) - epoch) > 0)
				{
					return;
				}

				if (state_.compare_exchange_weak(state, state - PREWAITER_INC + EPOCH_INC, std::memory_order_relaxed))
				{
					return;
				}
			}
		}

		void notify_one()
		{
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Uint64 state = state_.load(std::memory_order_acquire);
			for (;;)
			{
				if ((state & STACK_MASK) == STACK_MASK && (state & PREWAITER_MASK) == 0)
				{
					return;
				}

				Uint64 numberPrewaiters = (state & PREWAITER_MASK) >> PREWAITER_SHIFT;
				Uint64 newstate;

				if (numberPrewaiters)
				{
					newstate = state + EPOCH_INC - PREWAITER_INC;
				}
				else
				{
					Waiter* waiter = &waiters_[state & STACK_MASK];
					Waiter* nextWaiter = waiter->next_.load(std::memory_order_relaxed);
					Uint64 next = STACK_MASK;

					if (nextWaiter != nullptr)
					{
						next = static_cast<Uint64>(nextWaiter - &waiters_[0]);
					}
					newstate = (state & EPOCH_MASK) + next;
				}

				if (state_.compare_exchange_weak(state, newstate, std::memory_order_acquire))
				{
					if (numberPrewaiters)
					{
						return;
					}

					Waiter* waiter = &waiters_[state & STACK_MASK];
					waiter->next_.store(nullptr, std::memory_order_relaxed);
					unpark(waiter);
					return;
				}
			}
		}

		void notify_all()
		{
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Uint64 state = state_.load(std::memory_order_acquire);
			for (;;)
			{
				if ((state & STACK_MASK) == STACK_MASK && (state & PREWAITER_MASK) == 0)
				{
					return;
				}
				Uint64 numberPrewaiters = (state & PREWAITER_MASK) >> PREWAITER_SHIFT;
				Uint64 newstate = (state & EPOCH_MASK) + (EPOCH_INC * numberPrewaiters) + STACK_MASK;

				if (state_.compare_exchange_weak(state, newstate, std::memory_order_acquire))
				{
					if ((state & STACK_MASK) == STACK_MASK)
					{
						return;
					}

					Waiter* waiter = &waiters_[state & STACK_MASK];
					unpark(waiter);
					return;
				}
			}
		}

		void notify_count(Size n)
		{
			if (n == 0)
			{
				return;
			}

			if (n >= waiters_.size())
			{
				notify_all();
				return;
			}

			std::atomic_thread_fence(std::memory_order_seq_cst);
			Uint64 state = state_.load(std::memory_order_acquire);

			do
			{
				if ((state & STACK_MASK) == STACK_MASK && (state & PREWAITER_MASK) == 0)
				{
					return;
				}
				Uint64 numberPrewaiters = (state & PREWAITER_MASK) >> PREWAITER_SHIFT;
				Uint64 newstate;
				Size newcount;

				if (numberPrewaiters)
				{
					Size toUnblock = (n < numberPrewaiters) ? n : numberPrewaiters;
					newstate = state + (EPOCH_INC * toUnblock) - (PREWAITER_INC * toUnblock);
					newcount = n - toUnblock;
				}
				else
				{
					Waiter* waiter = &waiters_[state & STACK_MASK];
					Waiter* nextWaiter = waiter->next_.load(std::memory_order_relaxed);
					Uint64 next = STACK_MASK;

					if (nextWaiter != nullptr)
					{
						next = static_cast<Uint64>(nextWaiter - &waiters_[0]);
					}

					newstate = (state & EPOCH_MASK) + next;
					newcount = n - 1;
				}

				if (state_.compare_exchange_weak(state, newstate, std::memory_order_acquire))
				{
					n = newcount;
					if (numberPrewaiters == 0)
					{
						Waiter* waiter = &waiters_[state & STACK_MASK];
						waiter->next_.store(nullptr, std::memory_order_relaxed);
						unpark(waiter);
					}
				}
			} while (n > 0);
		}

		Size size()const
		{
			return waiters_.size();
		}

	private:
		std::atomic<Uint64> state_;

		DynamicArray<Waiter> waiters_;

		void park(Waiter* waiter)
		{
			Unsigned target = Waiter::NonSignaled;
			if (waiter->state_.compare_exchange_strong(target, Waiter::Waiting, std::memory_order_relaxed, std::memory_order_relaxed))
			{
				waiter->state_.wait(Waiter::Waiting, std::memory_order_relaxed);
			}
		}

		void unpark(Waiter* waiters)
		{
			Waiter* next = nullptr;
			for (Waiter* waiter = waiters;waiter;waiter = next)
			{
				next = waiter->next_.load(std::memory_order_relaxed);

				if (waiter->state_.exchange(Waiter::Signaled, std::memory_order_relaxed) == Waiter::Waiting)
				{
					waiter->state_.notify_one();
				}
			}
		}
	};
}