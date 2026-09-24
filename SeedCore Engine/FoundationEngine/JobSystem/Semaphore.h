#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* A counting semaphore used to limit how many tasks of one kind run at
	* once within the job system. A task (JobNode) that finds no unit free
	* is parked as a waiter, and is handed back to the scheduler the next
	* time any unit is released, to try again.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョブシステム内で、ある種類のタスクが同時に走る数を制限するための
	* カウンティングセマフォ。空きが無かったタスク（JobNode）は待機者と
	* して止めておき、次にどれかが解放されたときにスケジューラへ戻して
	* もう一度試させる。
	*/
	class Semaphore
	{
	private:
		/// [EN] Only a node acquires and releases, through JobNode::AcquireAll/ReleaseAll.
		/// [JP] 獲得と解放を行うのはノードだけで、JobNode::AcquireAll/ReleaseAll を通す。
		friend class JobNode;

	public:
		/**
		* [EN]
		* Constructs a semaphore with a maximum (and initial available)
		* value of zero; reset(Size) gives it a real limit later.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最大値（および初期の利用可能値）が 0 のセマフォを構築する。
		* 実際の上限は、後で reset(Size) で与える。
		*/
		Semaphore() = default;

		/**
		* [EN]
		* Constructs the semaphore with the given maximum (and initial
		* available) value.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定された最大値（および初期の利用可能値）でセマフォを構築する。
		*/
		explicit Semaphore(Size maxValue);

		/**
		* [EN]
		* Returns the current available value of the semaphore.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* セマフォの現在の利用可能な値を返す。
		*/
		Size value()const;

		/**
		* [EN]
		* Returns the maximum value the semaphore was configured with.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* セマフォに設定されている最大値を返す。
		*/
		Size max_value()const;

		/**
		* [EN]
		* Resets the semaphore's current value back to its existing
		* maximum value and forgets every waiting task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* セマフォの現在値を既存の最大値に戻し、待機中のタスクを全て忘れる。
		*/
		void reset();

		/**
		* [EN]
		* Resets the semaphore with a new maximum value, replacing the
		* previous maximum and current value and forgetting every waiting
		* task.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しい最大値でセマフォをリセットし、以前の最大値と現在値を
		* 置き換え、待機中のタスクを全て忘れる。
		*/
		void reset(Size newMaxValue);

	private:
		/// [EN] Protects all mutable state of the semaphore (maxValue_, currentValue_, waiters_) from concurrent access.
		/// [JP] セマフォの可変な状態（maxValue_, currentValue_, waiters_）を並行アクセスから保護する。
		mutable std::mutex mutex_;

		/// [EN] The configured maximum value of the semaphore.
		/// [JP] セマフォに設定された最大値。
		Size maxValue_ = 0;

		/// [EN] The current available value of the semaphore; decremented on acquire and incremented on release.
		/// [JP] セマフォの現在の利用可能値。acquire で減少し、release で増加する。
		Size currentValue_ = 0;

		/// [EN] Tasks that found no unit free and are parked until the next release.
		/// [JP] 空きが無かったため、次の release まで止めておくタスクの一覧。
		HybridArray<JobNode*> waiters_;

		/**
		* [EN]
		* Takes one unit for node and returns true, or, when none is free,
		* parks node as a waiter and returns false.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node のために1つ取って true を返す。空きが無ければ node を待機者
		* として止めておき、false を返す。
		*/
		Bool try_acquire_or_wait(JobNode* node);

		/**
		* [EN]
		* Gives one unit back to the semaphore and hands every task that was
		* waiting on it to nodes. Those tasks are not granted the semaphore
		* here: the caller reschedules them, and each one tries to acquire
		* again when it runs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* セマフォに1つ返し、待っていたタスクを全て nodes へ渡す。ここで
		* それらにセマフォを与えるわけではない。呼び出し側がスケジュールし
		* 直し、各タスクは実行時にもう一度獲得を試みる。
		*/
		void release(HybridArray<JobNode*>& nodes);
	};
}