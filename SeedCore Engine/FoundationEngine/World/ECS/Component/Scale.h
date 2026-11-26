#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct Scale
	{
		SC_SERIALIZE_FIELD()
		Float x_;

		SC_SERIALIZE_FIELD()
		Float y_;

		SC_SERIALIZE_FIELD()
		Float z_;
	};
	REGISTER_COMPONENT(Scale, "Core", ComponentStorage::Archetype);
}
