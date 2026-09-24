#pragma once
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/Log/Exeption.h>
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	/**
	* [EN]
	* Lock-free multi-waiter notifier (the classic "EventCount" design).
	* Unlike AtomicNotifier, which waits on one shared word, each thread
	* parks on its own Waiter's atomic, so notify_one wakes exactly the
	* thread it picked instead of every sleeper. A single 64-bit
	* state_ packs three fields: an epoch (top EPOCH_BITS bits, bumped on
	* every notify so a race between checking a condition and parking is
	* never missed), a "prewaiter" count (middle PREWAITER_BITS bits, for
	* threads that called prepare_wait but haven't committed to actually
	* parking yet — waking those is cheaper than touching the stack), and
	* the index of the top of an intrusive LIFO stack of currently-parked
	* Waiters (low STACK_BITS bits, STACK_MASK sentinel = empty). Used as
	* the job system's default notifier unless SC_ENABLE_ATOMIC_NOTIFIER
	* selects AtomicNotifier instead (see WorkerCommon.h).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ロックフリーな複数待機者向け notifier（古典的な「EventCount」設計）。
	* 共有の1ワードで待つ AtomicNotifier と異なり、各スレッドは自分の
	* Waiter のアトミックで眠るので、notify_one は眠っている全員ではなく、
	* 選んだスレッドだけを起こす。
	* 単一の64ビット state_ に3つのフィールドを詰め込む: エポック
	* （上位 EPOCH_BITS ビット。notify のたびに進められ、条件チェックと
	* パークの間の競合を見逃さないようにする）、「prewaiter」数
	* （中位 PREWAITER_BITS ビット。prepare_wait を呼んだが、まだ実際の
	* パークを確約していないスレッド用 — これらの起床はスタックに
	* 触れるより安価）、そして現在パーク中の Waiter からなる侵入型LIFO
	* スタックの先頭インデックス（下位 STACK_BITS ビット、STACK_MASK が
	* 番兵で空を意味する）。WorkerCommon.h の SC_ENABLE_ATOMIC_NOTIFIER が
	* AtomicNotifier を選択しない限り、ジョブシステムの既定 notifier として使われる。
	*/
	class NonblockingNotifier
	{
	private:
		/// [EN] Grants the executor direct access to the waiter slots.
		/// [JP] JobExecutor に待機者スロットへの直接アクセスを許す。
		friend class JobExecutor;

		/**
		* [EN]
		* Intrusive stack node for a parked thread: a next_ link for the
		* LIFO parked-waiter stack, the epoch snapshot recorded at
		* prepare_wait, and a small state machine driving the actual
		* futex-style park/unpark.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パーク中スレッド用の侵入型スタックノード。LIFOパーク待機
		* スタック用の next_ リンク、prepare_wait で記録したエポック
		* スナップショット、実際のfutex風パーク/アンパークを駆動する
		* 小さなステートマシンを持つ。
		*/
		struct Waiter
		{
			/// [EN] Link to the next Waiter down the intrusive parked-waiter stack.
			/// [JP] 侵入型パーク待機スタックの次の Waiter へのリンク。
			alignas(SC_CACHELINE_SIZE) std::atomic<Waiter*> next_;

			/// [EN] Combined epoch/prewaiter snapshot recorded at prepare_wait, used by commit_wait to detect a missed notify.
			/// [JP] prepare_wait で記録された、エポック/prewaiterを合成したスナップショット。commit_wait が notify の見逃しを検出するために使う。
			Uint64 epoch_;

			/// [EN] park/unpark state machine values.
			/// [JP] park/unpark のステートマシンの値。
			enum : Unsigned
			{
				/// [EN] Not yet parked and not signaled.
				/// [JP] まだパークされておらず、シグナルもされていない。
				NonSignaled = 0,

				/// [EN] Actually blocked in park(), waiting to be woken.
				/// [JP] park() で実際にブロック中で、起床を待っている。
				Waiting,

				/// [EN] Woken (or about to be, racing with park()) by unpark().
				/// [JP] unpark() によって起床済み（または park() と競合中で起床直前）。
				Signaled,
			};

			/// [EN] Current park/unpark state.
			/// [JP] 現在の park/unpark 状態。
			std::atomic<Unsigned> state_{ 0 };
		};

	public:
		/// [EN] Bit width of the parked-waiter stack-top index field within state_.
		/// [JP] state_ 内のパーク待機スタック先頭インデックスフィールドのビット幅。
		static const Uint64 STACK_BITS = 16;

		/// [EN] Mask isolating the stack-top index field (also its "empty stack" sentinel value).
		/// [JP] スタック先頭インデックスフィールドを取り出すマスク（「空スタック」の番兵値でもある）。
		static const Uint64 STACK_MASK = (1ull << STACK_BITS) - 1;

		/// [EN] Bit width of the prewaiter-count field within state_.
		/// [JP] state_ 内のprewaiter数フィールドのビット幅。
		static const Uint64 PREWAITER_BITS = 16;

		/// [EN] Bit position where the prewaiter-count field begins within state_.
		/// [JP] state_ 内でprewaiter数フィールドが始まるビット位置。
		static const Uint64 PREWAITER_SHIFT = 16;

		/// [EN] Mask isolating the prewaiter-count field.
		/// [JP] prewaiter数フィールドを取り出すマスク。
		static const Uint64 PREWAITER_MASK = ((1ull << PREWAITER_BITS) - 1) << PREWAITER_SHIFT;

		/// [EN] Amount to add to/subtract from state_ to change the prewaiter count by one.
		/// [JP] prewaiter数を1増減させるために state_ に加減算する量。
		static const Uint64 PREWAITER_INC = 1ull << PREWAITER_BITS;

		/// [EN] Bit width of the epoch field within state_.
		/// [JP] state_ 内のエポックフィールドのビット幅。
		static const Uint64 EPOCH_BITS = 32;

		/// [EN] Bit position where the epoch field begins within state_.
		/// [JP] state_ 内でエポックフィールドが始まるビット位置。
		static const Uint64 EPOCH_SHIFT = 32;

		/// [EN] Mask isolating the epoch field.
		/// [JP] エポックフィールドを取り出すマスク。
		static const Uint64 EPOCH_MASK = ((1ull << EPOCH_BITS) - 1) << EPOCH_SHIFT;

		/// [EN] Amount to add to state_ to advance the epoch by one.
		/// [JP] エポックを1進めるために state_ に加算する量。
		static const Uint64 EPOCH_INC = 1ull << EPOCH_SHIFT;

		/**
		* [EN]
		* Constructs a notifier with n waiter slots (index range [0, n)
		* passed to prepare_wait/commit_wait/cancel_wait), starting with
		* an empty parked-waiter stack. Throws if n exceeds what
		* PREWAITER_BITS can represent.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* n個の待機者スロットを持つ notifier を構築する
		* （prepare_wait/commit_wait/cancel_wait に渡すインデックス範囲は
		* [0, n)）。パーク待機スタックは空の状態で開始する。n が
		* PREWAITER_BITS で表現できる範囲を超える場合は例外を送出する。
		*/
		explicit NonblockingNotifier(Size n) :state_(STACK_MASK), waiters_(n)
		{
			/// [EN] Every waiter may be a prewaiter at once, so the count field must be able to hold n.
			/// [JP] 全待機者が同時に prewaiter になりうるので、数のフィールドは n を表せる必要がある。
			if (waiters_.size() >= ((1 << PREWAITER_BITS) - 1))
			{
				SC_THROW("NonblockingNotifier で設定可能な待機スレッド数は最大 {} 個までです。", (1 << PREWAITER_BITS) - 1);
			}
		}

		/**
		* [EN]
		* Asserts that the parked-waiter stack is empty and no prewaiters
		* remain before destruction.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 破棄前に、パーク待機スタックが空で、prewaiterも残っていないことをアサートする。
		*/
		~NonblockingNotifier()
		{
			/// [EN] The epoch may hold anything; only the stack and the prewaiter count must be empty.
			/// [JP] エポックはどんな値でもよい。空でなければならないのはスタックと prewaiter の数だけ。
			SC_ASSERT((state_.load() & (STACK_MASK | PREWAITER_MASK)) == STACK_MASK);
		}

		/**
		* [EN]
		* Returns the number of waiters currently actually parked (O(n)
		* scan over every slot's state).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在実際にパークされている待機者数を返す（全スロットの状態を
		* 走査する O(n) 処理）。
		*/
		Size count()const
		{
			/// [EN] Only threads really blocked in park() are counted; prewaiters are not.
			/// [JP] 数えるのは park() で本当に止まっているスレッドだけで、prewaiter は含めない。
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

		/**
		* [EN]
		* Returns the maximum number of waiters representable by the
		* parked-waiter stack index field.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パーク待機スタックのインデックスフィールドで表現可能な、
		* 最大待機者数を返す。
		*/
		Size capacity()const
		{
			return 1 << STACK_BITS;
		}

		/**
		* [EN]
		* Registers wid as a prewaiter and records the current
		* epoch/prewaiter snapshot. Must be paired with a later
		* commit_wait or cancel_wait on the same wid. Call this before
		* re-checking whatever condition you're waiting on, so a notify
		* racing with the check is still observed via the epoch bump.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* wid を prewaiter として登録し、現在のエポック/prewaiter
		* スナップショットを記録する。同じ wid に対して、後で commit_wait
		* または cancel_wait と対にする必要がある。待機対象の条件を
		* 再チェックする前にこれを呼ぶことで、チェックと競合する notify
		* があってもエポックの進行によって見逃さないようにする。
		*/
		void prepare_wait(Size wid)
		{
			/// [EN] Joining the prewaiters and snapshotting the state happen in one atomic step.
			/// [JP] prewaiter への参加と状態の記録を、1回の不可分な操作で行う。
			waiters_[wid].epoch_ = state_.fetch_add(PREWAITER_INC, std::memory_order_relaxed);

			/// [EN] Pairs with the fence in notify_*, so either the notifier sees this prewaiter or the caller's re-check sees the new work.
			/// [JP] notify_* 側のフェンスと対になる。通知側がこの prewaiter を見るか、呼び出し側の再確認が新しい仕事を見るかのどちらかになる。
			std::atomic_thread_fence(std::memory_order_seq_cst);
		}

		/**
		* [EN]
		* Actually parks wid's thread unless a notify already advanced the
		* epoch past what prepare_wait recorded (in which case it returns
		* immediately). Otherwise, CASes itself onto the top of the
		* intrusive parked-waiter stack and blocks in park().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* prepare_wait が記録した時点より notify がエポックを進めていない
		* 限り、wid のスレッドを実際にパークする（既に進んでいれば
		* 即座に返る）。それ以外の場合は、侵入型パーク待機スタックの
		* 先頭に自身をCASで積み、park() でブロックする。
		*/
		void commit_wait(Size wid)
		{
			auto waiter = &waiters_[wid];
			waiter->state_.store(Waiter::NonSignaled, std::memory_order_relaxed);

			/// [EN] This prewaiter's turn: the recorded epoch plus the number of prewaiters that registered before it.
			/// [JP] この prewaiter の番。記録したエポックに、先に登録していた prewaiter の数を足したもの。
			Uint64 epoch = (waiter->epoch_ & EPOCH_MASK) + (((waiter->epoch_ & PREWAITER_MASK) >> PREWAITER_SHIFT) << EPOCH_SHIFT);
			Uint64 state = state_.load(std::memory_order_seq_cst);
			for (;;)
			{
				/// [EN] The epoch has not reached this turn yet: earlier prewaiters are still settling, so wait for them.
				/// [JP] エポックがまだこの番に届いていない。先の prewaiter が片付くのを待つ。
				if (Int64((state & EPOCH_MASK) - epoch) < 0)
				{
					std::this_thread::yield();
					state = state_.load(std::memory_order_seq_cst);
					continue;
				}

				/// [EN] The epoch is past this turn: a notify already consumed this prewaiter, so there is no need to sleep.
				/// [JP] エポックがこの番を過ぎている。通知が既にこの prewaiter を消費したので、眠る必要は無い。
				if (Int64((state & EPOCH_MASK) - epoch) > 0)
				{
					return;
				}

				/// [EN] Leaves the prewaiters, advances the epoch for the next one, and becomes the new top of the parked stack.
				/// [JP] prewaiter から抜け、次の番のためにエポックを進め、パーク中スタックの新しい先頭になる。
				Uint64 newstate = state - PREWAITER_INC + EPOCH_INC;
				newstate = (newstate & ~STACK_MASK) | wid;

				/// [EN] next_ links to the previous top so the stack stays intact.
				/// [JP] next_ を前の先頭へつなぎ、スタックを保つ。
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

			/// [EN] Now reachable from the stack, so a notifier can find this waiter and unpark it.
			/// [JP] スタックから辿れるようになったので、通知側がこの待機者を見つけて起こせる。
			park(waiter);
		}

		/**
		* [EN]
		* Cancels a pending prepare_wait without actually parking (e.g.
		* the caller found work to do before calling commit_wait), by
		* bumping the epoch and decrementing the prewaiter count. No-op
		* if a notify already consumed this prewaiter slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 実際にはパークせずに、保留中の prepare_wait を取り消す
		* （例: commit_wait を呼ぶ前に呼び出し側が実行すべき仕事を見つけた
		* 場合）。エポックを進め、prewaiter数をデクリメントする。既に
		* notify がこの prewaiter スロットを消費していれば何もしない。
		*/
		void cancel_wait(Size wid)
		{
			/// [EN] The same turn computation as commit_wait.
			/// [JP] commit_wait と同じ番の求め方。
			Uint64 epoch = (waiters_[wid].epoch_ & EPOCH_MASK) + (((waiters_[wid].epoch_ & PREWAITER_MASK) >> PREWAITER_SHIFT) << EPOCH_SHIFT);
			Uint64 state = state_.load(std::memory_order_relaxed);
			for (;;)
			{
				/// [EN] Earlier prewaiters are still settling, so wait for this turn.
				/// [JP] 先の prewaiter がまだ片付いていないので、この番を待つ。
				if (Int64((state & EPOCH_MASK) - epoch) < 0)
				{
					std::this_thread::yield();
					state = state_.load(std::memory_order_relaxed);
					continue;
				}

				/// [EN] A notify already consumed this prewaiter and did the bookkeeping for it.
				/// [JP] 通知が既にこの prewaiter を消費し、後始末も済ませている。
				if (Int64((state & EPOCH_MASK) - epoch) > 0)
				{
					return;
				}

				/// [EN] Leaves the prewaiters and advances the epoch, handing the turn to the next prewaiter.
				/// [JP] prewaiter から抜けてエポックを進め、次の prewaiter に番を渡す。
				if (state_.compare_exchange_weak(state, state - PREWAITER_INC + EPOCH_INC, std::memory_order_relaxed))
				{
					return;
				}
			}
		}

		/**
		* [EN]
		* Wakes at most one waiter: prefers consuming a pending prewaiter
		* (cheap: only bumps epoch and decrements the prewaiter count) if
		* any exist, otherwise pops and unparks the top of the intrusive
		* parked-waiter stack. No-op if nothing is waiting.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最大1つの待機者を起床させる: 保留中の prewaiter があればそれを
		* 消費することを優先する（安価: エポックを進め prewaiter数を
		* デクリメントするだけ）。なければ侵入型パーク待機スタックの先頭を
		* pop してアンパークする。何も待機していなければ何もしない。
		*/
		void notify_one()
		{
			/// [EN] Pairs with the fence in prepare_wait, so work published before this call is visible to the woken thread.
			/// [JP] prepare_wait 側のフェンスと対になり、この呼び出しより前に積んだ仕事が起きたスレッドから見える。
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Uint64 state = state_.load(std::memory_order_acquire);
			for (;;)
			{
				/// [EN] An empty stack and no prewaiter mean nobody is waiting, so there is nothing to do.
				/// [JP] スタックが空で prewaiter もいなければ、待っている者はいないので何もしない。
				if ((state & STACK_MASK) == STACK_MASK && (state & PREWAITER_MASK) == 0)
				{
					return;
				}

				Uint64 numberPrewaiters = (state & PREWAITER_MASK) >> PREWAITER_SHIFT;
				Uint64 newstate;

				/// [EN] A prewaiter has not slept yet, so consuming one is just a state change: it sees the new epoch and does not park.
				/// [JP] prewaiter はまだ眠っていないので、消費するのは状態の変更だけで済む。新しいエポックを見て眠らずに済む。
				if (numberPrewaiters)
				{
					newstate = state + EPOCH_INC - PREWAITER_INC;
				}
				else
				{
					/// [EN] No prewaiter: the top parked waiter is popped, and its next_ becomes the new top (or the empty sentinel).
					/// [JP] prewaiter がいないので、パーク中の先頭を取り出し、その next_ を新しい先頭（または空の番兵）にする。
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

					/// [EN] next_ is cut so unpark wakes only the popped waiter, not the rest of the stack.
					/// [JP] next_ を切って、unpark がスタックの残りではなく取り出した待機者だけを起こすようにする。
					Waiter* waiter = &waiters_[state & STACK_MASK];
					waiter->next_.store(nullptr, std::memory_order_relaxed);
					unpark(waiter);
					return;
				}
			}
		}

		/**
		* [EN]
		* Wakes every prewaiter and every parked waiter: advances the
		* epoch past all prewaiters, empties the parked-waiter stack, and
		* unparks the entire popped chain.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全ての prewaiter とパーク中の全待機者を起床させる: エポックを
		* 全prewaiter分進め、パーク待機スタックを空にし、popされた
		* チェーン全体をアンパークする。
		*/
		void notify_all()
		{
			/// [EN] Pairs with the fence in prepare_wait, so work published before this call is visible to the woken threads.
			/// [JP] prepare_wait 側のフェンスと対になり、この呼び出しより前に積んだ仕事が起きたスレッドから見える。
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Uint64 state = state_.load(std::memory_order_acquire);
			for (;;)
			{
				/// [EN] An empty stack and no prewaiter mean nobody is waiting, so there is nothing to do.
				/// [JP] スタックが空で prewaiter もいなければ、待っている者はいないので何もしない。
				if ((state & STACK_MASK) == STACK_MASK && (state & PREWAITER_MASK) == 0)
				{
					return;
				}
				Uint64 numberPrewaiters = (state & PREWAITER_MASK) >> PREWAITER_SHIFT;
				/// [EN] Every prewaiter is consumed by one epoch step each, and the whole parked stack is detached at once.
				/// [JP] 全 prewaiter をエポック1つずつで消費し、パーク中のスタックは丸ごと一度に切り離す。
				Uint64 newstate = (state & EPOCH_MASK) + (EPOCH_INC * numberPrewaiters) + STACK_MASK;

				if (state_.compare_exchange_weak(state, newstate, std::memory_order_acquire))
				{
					if ((state & STACK_MASK) == STACK_MASK)
					{
						return;
					}

					/// [EN] The detached chain is walked through next_, waking every parked waiter on it.
					/// [JP] 切り離した連なりを next_ で辿り、そこにいるパーク中の待機者を全て起こす。
					Waiter* waiter = &waiters_[state & STACK_MASK];
					unpark(waiter);
					return;
				}
			}
		}

		/**
		* [EN]
		* Wakes up to n waiters total, consuming prewaiters first (cheaper)
		* and then popping from the parked-waiter stack until n is
		* satisfied or nothing remains waiting. Falls back to notify_all
		* if n covers every configured slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最大 n 個の待機者を起床させる。まず（より安価な）prewaiterを
		* 消費し、その後 n を満たすか待機者がいなくなるまでパーク待機
		* スタックから pop していく。n が設定済みの全スロットを覆う場合は
		* notify_all にフォールバックする。
		*/
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

			/// [EN] Pairs with the fence in prepare_wait, as in notify_one.
			/// [JP] notify_one と同じく、prepare_wait 側のフェンスと対になる。
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Uint64 state = state_.load(std::memory_order_acquire);

			/// [EN] Each round wakes a batch of prewaiters or one parked waiter, until n wakeups are done or nobody waits.
			/// [JP] 1回ごとに prewaiter をまとめて、あるいはパーク中の1つを起こし、n 回分終わるか誰も待っていなくなるまで続ける。
			do
			{
				/// [EN] An empty stack and no prewaiter mean nobody is waiting, so there is nothing to do.
				/// [JP] スタックが空で prewaiter もいなければ、待っている者はいないので何もしない。
				if ((state & STACK_MASK) == STACK_MASK && (state & PREWAITER_MASK) == 0)
				{
					return;
				}
				Uint64 numberPrewaiters = (state & PREWAITER_MASK) >> PREWAITER_SHIFT;
				Uint64 newstate;
				Size newcount;

				/// [EN] Prewaiters are consumed first, as many as still needed, because they cost only a state change.
				/// [JP] 状態の変更だけで済むので、まず prewaiter を必要な数だけ消費する。
				if (numberPrewaiters)
				{
					Size toUnblock = (n < numberPrewaiters) ? n : numberPrewaiters;
					newstate = state + (EPOCH_INC * toUnblock) - (PREWAITER_INC * toUnblock);
					newcount = n - toUnblock;
				}
				else
				{
					/// [EN] No prewaiter: the top parked waiter is popped, and its next_ becomes the new top (or the empty sentinel).
					/// [JP] prewaiter がいないので、パーク中の先頭を取り出し、その next_ を新しい先頭（または空の番兵）にする。
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

		/**
		* [EN]
		* Returns the total number of configured waiter slots.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 設定されている待機者スロットの総数を返す。
		*/
		Size size()const
		{
			return waiters_.size();
		}

	private:
		/// [EN] Packed state: epoch (EPOCH_MASK), prewaiter count (PREWAITER_MASK) and parked-stack top index (STACK_MASK).
		/// [JP] 詰め込まれた状態。エポック（EPOCH_MASK）、prewaiter 数（PREWAITER_MASK）、パーク中スタックの先頭インデックス（STACK_MASK）。
		std::atomic<Uint64> state_;

		/// [EN] Waiter slots, indexed by the wid passed to prepare_wait/commit_wait/cancel_wait.
		/// [JP] Waiterスロット。prepare_wait/commit_wait/cancel_wait に渡す wid でアクセスする。
		DynamicArray<Waiter> waiters_;

		/**
		* [EN]
		* Blocks waiter until unparked, unless it was already marked
		* Signaled by a racing unpark() (in which case this returns
		* immediately without blocking).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アンパークされるまで waiter をブロックする。ただし、競合する
		* unpark() によって既に Signaled とマークされていた場合は
		* ブロックせず即座に返る。
		*/
		void park(Waiter* waiter)
		{
			/// [EN] Moving to Waiting fails only if unpark already signaled this waiter, in which case there is nothing to wait for.
			/// [JP] Waiting への移行が失敗するのは unpark が既に知らせていた場合だけで、そのときは待つものが無い。
			Unsigned target = Waiter::NonSignaled;
			if (waiter->state_.compare_exchange_strong(target, Waiter::Waiting, std::memory_order_relaxed, std::memory_order_relaxed))
			{
				/// [EN] wait() returns once the state is no longer Waiting.
				/// [JP] wait() は状態が Waiting でなくなった時点で戻る。
				waiter->state_.wait(Waiter::Waiting, std::memory_order_relaxed);
			}
		}

		/**
		* [EN]
		* Walks the intrusive next_ chain starting at waiters (as popped
		* from the parked-waiter stack) and wakes each Waiter that was
		* actually blocked in park().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* waiters（パーク待機スタックから pop されたもの）から始まる
		* 侵入型 next_ チェーンを辿り、実際に park() でブロックされていた
		* 各 Waiter を起床させる。
		*/
		void unpark(Waiter* waiters)
		{
			Waiter* next = nullptr;
			for (Waiter* waiter = waiters;waiter;waiter = next)
			{
				/// [EN] next_ is read before signaling, because a woken waiter may reuse its slot straight away.
				/// [JP] 知らせる前に next_ を読んでおく。起きた待機者はすぐに自分のスロットを使い直すことがあるため。
				next = waiter->next_.load(std::memory_order_relaxed);

				/// [EN] Only a thread already blocked needs a notify; one still on its way to park() sees Signaled and skips waiting.
				/// [JP] 通知が要るのは既に止まっているスレッドだけ。park() へ向かう途中のスレッドは Signaled を見て待たずに済む。
				if (waiter->state_.exchange(Waiter::Signaled, std::memory_order_relaxed) == Waiter::Waiting)
				{
					waiter->state_.notify_one();
				}
			}
		}
	};
}