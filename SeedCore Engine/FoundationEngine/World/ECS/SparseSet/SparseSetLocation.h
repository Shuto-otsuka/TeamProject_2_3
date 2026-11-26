#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	struct SparseSetLocation
	{
		ComponentID id_ = nullptr;

		Uint32 row_ = UINT32_MAX;

		void* data_ = nullptr;
	};
}