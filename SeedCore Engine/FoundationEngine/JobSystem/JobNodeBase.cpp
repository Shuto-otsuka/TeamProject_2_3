#include <FoundationEngine/JobSystem/JobNodeBase.h>

namespace SeedCore
{
	/**
	* [EN]
	* Explicit constructor for initializing all fields at once.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全フィールドを一括初期化するための明示的コンストラクタ。
	*/
	JobNodeBase::JobNodeBase(NState nstate, EState estate, JobNodeBase* parent, Size joinCounter) :nstate_(nstate), estate_(estate), parent_(parent), joinCounter_(joinCounter)
	{
		/// No Code
	}

	/**
	* [EN]
	* Clears and rethrows the exception stored on this node, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードに格納された例外があれば、消してから投げ直す。
	*/
	void JobNodeBase::RethrowException()
	{
		if (exceptionPtr_)
		{
			/// [EN] The node is reset first, so it can catch a new exception the next time it is waited on.
			/// [JP] 先にノードを元に戻しておき、次に待つときに新しい例外を受け取れるようにする。
			std::exception_ptr exception = exceptionPtr_;
			exceptionPtr_ = nullptr;
			estate_.fetch_and(~(JobExceptionState::EXCEPTION | JobExceptionState::CAUGHT), std::memory_order_relaxed);
			std::rethrow_exception(exception);
		}
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Sets EXPLICITLY_ANCHORED on node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node に EXPLICITLY_ANCHORED を立てる。
	*/
	JobExplicitAnchorGuard::JobExplicitAnchorGuard(JobNodeBase* node) :node_(node)
	{
		/// [EN] The flag lives in estate_ (atomic), because other workers read it while the caller is blocked.
		/// [JP] 呼び出し側が待っている間に他のワーカーが読むので、フラグはアトミックな estate_ に置く。
		node_->estate_.fetch_or(JobExceptionState::EXPLICITLY_ANCHORED, std::memory_order_relaxed);
	}

	/**
	* [EN]
	* Clears EXPLICITLY_ANCHORED from the node again.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node から EXPLICITLY_ANCHORED を再び消す。
	*/
	JobExplicitAnchorGuard::~JobExplicitAnchorGuard()
	{
		node_->estate_.fetch_and(~JobExceptionState::EXPLICITLY_ANCHORED, std::memory_order_relaxed);
	}
}