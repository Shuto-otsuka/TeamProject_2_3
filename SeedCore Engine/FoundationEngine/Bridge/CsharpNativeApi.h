#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	struct CsharpNativeApi
	{
		void (*log_)(Uint8 level, const Uint8* text, Int32 length) = nullptr;
	};
}