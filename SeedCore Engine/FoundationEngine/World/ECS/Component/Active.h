#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct Active
	{
		Bool active_ = true;
	};
	REGISTER_COMPONENT(Active, "Core", ComponentStorage::Archetype);
}
