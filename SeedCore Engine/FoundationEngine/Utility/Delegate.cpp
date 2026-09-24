#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Delegate.h>
#include <atomic>

namespace SeedCore
{
	/**
	* [EN]
	* Returns a freshly generated, globally unique handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 新たに発行された、グローバルに一意なハンドルを返す。
	*/
	DelegateHandle DelegateHandle::Generate()
	{
		/// [EN] Process-wide counter starting at 1, so 0 stays reserved for "invalid".
		/// [JP] プロセス全体で共有するカウンタ。0 を「無効」用に残すため 1 から始める。
		static std::atomic<Uint64> nextId = 1;

		/// [EN] fetch_add hands out distinct ids even when several threads bind at once; no ordering is needed beyond that.
		/// [JP] fetch_add は複数のスレッドが同時にバインドしても別々の id を配る。それ以上の順序付けは要らない。
		DelegateHandle handle;
		handle.id_ = nextId.fetch_add(1, std::memory_order_relaxed);
		return handle;
	}

	/**
	* [EN]
	* Returns whether this handle was produced by Generate() (as
	* opposed to being default-constructed).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このハンドルが（デフォルト構築ではなく）Generate() によって
	* 発行されたものかどうかを返す。
	*/
	Bool DelegateHandle::Valid()const
	{
		return id_ != 0;
	}
}
