#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct Lifetime
	{
		SC_REFLECTION_CLAMPED_EX("生存時間(秒)", 0.05f, 3600.0f)
		Float duration_ = 5.0f;
	};
	REGISTER_COMPONENT(Lifetime, "Gameplay");
}
