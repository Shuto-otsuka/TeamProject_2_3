#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Delegate.h>
#include <atomic>

namespace SeedCore
{
	DelegateHandle DelegateHandle::Generate()
	{
		static std::atomic<Uint64> nextId = 1;

		DelegateHandle handle;
		handle.id_ = nextId.fetch_add(1, std::memory_order_relaxed);
		return handle;
	}

	Bool DelegateHandle::Valid()const
	{
		return id_ != 0;
	}
}
