#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	SynchronizedPointer::SynchronizedPointer(PointerType ptr, TagType tag)noexcept :ptr_(ptr), tag_(tag)
	{
		/// No Code
	}
}