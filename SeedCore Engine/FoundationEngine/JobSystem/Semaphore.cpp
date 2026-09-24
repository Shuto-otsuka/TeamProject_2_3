#include <FoundationEngine/JobSystem/Semaphore.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
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
	Semaphore::Semaphore(Size maxValue) :maxValue_(maxValue), currentValue_(maxValue)
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns the current available value of the semaphore.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* セマフォの現在の利用可能な値を返す。
	*/
	Size Semaphore::value()const
	{
		/// [EN] The count is changed by other threads, so it is read under the lock.
		/// [JP] 値は他のスレッドから変えられるので、ロックの内側で読む。
		std::lock_guard<std::mutex> lock(mutex_);
		return currentValue_;
	}

	/**
	* [EN]
	* Returns the maximum value the semaphore was configured with.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* セマフォに設定されている最大値を返す。
	*/
	Size Semaphore::max_value()const
	{
		/// [EN] Read without the lock: the maximum only changes through reset, which is not meant to run alongside tasks.
		/// [JP] ロックなしで読む。最大値が変わるのは reset のときだけで、タスクの実行と並行して呼ぶものではない。
		return maxValue_;
	}

	/**
	* [EN]
	* Resets the semaphore's current value back to its existing maximum
	* value and forgets every waiting task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* セマフォの現在値を既存の最大値に戻し、待機中のタスクを全て忘れる。
	*/
	void Semaphore::reset()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		currentValue_ = maxValue_;

		/// [EN] Waiters are dropped rather than handed back, since nothing is waiting on a fresh semaphore.
		/// [JP] 待機者は戻さずにそのまま捨てる。作り直したセマフォを待っているものは無いため。
		waiters_.clear();
	}

	/**
	* [EN]
	* Resets the semaphore with a new maximum value, replacing the
	* previous maximum and current value and forgetting every waiting task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 新しい最大値でセマフォをリセットし、以前の最大値と現在値を
	* 置き換え、待機中のタスクを全て忘れる。
	*/
	void Semaphore::reset(Size newMaxValue)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		/// [EN] The semaphore starts fully available under its new limit.
		/// [JP] 新しい上限のもとで、全て空いた状態から始める。
		currentValue_ = (maxValue_ = newMaxValue);

		/// [EN] Waiters are dropped rather than handed back, since nothing is waiting on a fresh semaphore.
		/// [JP] 待機者は戻さずにそのまま捨てる。作り直したセマフォを待っているものは無いため。
		waiters_.clear();
	}

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
	Bool Semaphore::try_acquire_or_wait(JobNode* node)
	{
		/// [EN] Checking the count and parking happen under one lock, so a release in between cannot be missed.
		/// [JP] 値の確認と待機者への登録を1つのロックの中で行うので、その間の release を取りこぼさない。
		std::lock_guard<std::mutex> lock(mutex_);
		if (currentValue_ > 0)
		{
			/// [EN] A unit is free: take it and let the caller run the task now.
			/// [JP] 空きがあるので1つ取り、呼び出し元にタスクを今すぐ実行させる。
			--currentValue_;
			return true;
		}
		else
		{
			/// [EN] No unit is free: park node until the next release, and tell the caller not to run it now.
			/// [JP] 空きが無いので次の release まで node を止めておき、呼び出し元には今は実行しないよう伝える。
			waiters_.push_back(node);
			return false;
		}
	}

	/**
	* [EN]
	* Gives one unit back and hands every waiting task to nodes, for the
	* caller to reschedule; each of them tries to acquire again.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つ返し、待っていたタスクを全て nodes へ渡す。呼び出し側がスケジュール
	* し直し、各タスクはもう一度獲得を試みる。
	*/
	void Semaphore::release(HybridArray<JobNode*>& nodes)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (currentValue_ >= maxValue_)
		{
			/// [EN] Every unit is already back, so this release has no matching acquire.
			/// [JP] 全て返却済みなので、この release には対応する acquire が無い。
			SC_THROW("Semaphore::release() が acquire 済みの回数を超えて呼び出されました。");
		}

		++currentValue_;

		/// [EN] Every waiter is handed back, not just one: they race to acquire again, and the losers simply park once more.
		/// [JP] 1つだけでなく全待機者を返す。もう一度獲得を競い、負けたものは再び待機者に戻るだけ。
		if (nodes.empty())
		{
			/// [EN] nodes is empty, so the waiter list is swapped in instead of copied.
			/// [JP] nodes が空なので、コピーせずに待機者リストと入れ替える。
			nodes.swap(waiters_);
		}
		else
		{
			/// [EN] nodes already holds tasks from another semaphore, so the waiters are appended after them.
			/// [JP] nodes には他のセマフォから来たタスクが既に入っているので、その後ろに待機者を足す。
			nodes.reserve(nodes.size() + waiters_.size());
			nodes.insert(nodes.end(), waiters_.begin(), waiters_.end());
			waiters_.clear();
		}
	}
}