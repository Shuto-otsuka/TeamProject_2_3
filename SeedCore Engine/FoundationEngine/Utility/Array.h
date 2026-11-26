#pragma once
#include <FoundationEngine/Utility/DynamicArray.h>
#include <FoundationEngine/Utility/StaticArray.h>
#include <FoundationEngine/JobSystem/JobVector.h>

namespace SeedCore
{
	template<typename T, Size N = 2>
		requires (N > 0)
	using HybridArray = JobVector<T, N>;
}
