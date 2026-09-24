#include <FoundationEngine/JobSystem/JobTopology.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	/**
	* [EN]
	* Destroys the topology and the taskflow it owns, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* トポロジーと、所有しているタスクフロー（あれば）を破棄する。
	*/
	JobTopology::~JobTopology()
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns whether this run has been cancelled or has failed with an
	* exception.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この実行がキャンセルされたか、例外で失敗しているかを返す。
	*/
	Bool JobTopology::Cancelled()const
	{
		/// [EN] A failed run is treated like a cancelled one, so it is not repeated.
		/// [JP] 失敗した実行はキャンセルされたものと同じ扱いにし、繰り返さない。
		return estate_.load(std::memory_order_relaxed) & (JobExceptionState::CANCELLED | JobExceptionState::EXCEPTION);
	}

	/**
	* [EN]
	* Fulfills promise_, with the stored exception if the run failed,
	* unblocking any JobFuture<void> observers waiting on this run.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* promise_ を満たし（実行が失敗していれば格納された例外で）、この
	* 実行を待機している JobFuture<void> オブザーバーのブロックを解除する。
	*/
	void JobTopology::CarryOutPromise()
	{
		if (exceptionPtr_)
		{
			/// [EN] The exception is cleared from the topology before being handed over, so the next run starts clean.
			/// [JP] 例外はトポロジーから外してから渡す。次の実行はまっさらな状態で始まる。
			auto exception = exceptionPtr_;
			exceptionPtr_ = nullptr;
			promise_.set_exception(exception);
		}
		else
		{
			/// [EN] No exception: report normal completion.
			/// [JP] 例外なし: 通常完了を報告する。
			promise_.set_value();
		}
	}
}
