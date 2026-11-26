#pragma once
#include <FoundationEngine/Resource/ResourceSync.h>

namespace SeedCore
{
	class BenchmarkSystem;

	struct ApplicationContext
	{
		ResourceSync* resourceSync_ = nullptr;

		BenchmarkSystem* benchmark_ = nullptr;

		Uint64 uiFrame_ = 0;

		Bool exitRequested_ = false;
	};
}
