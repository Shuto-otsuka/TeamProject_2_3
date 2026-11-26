#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct Name
	{
		SC_SERIALIZE_FIELD()
		String name_;
	};
	REGISTER_COMPONENT(Name, "Core", ComponentStorage::Archetype);
}
