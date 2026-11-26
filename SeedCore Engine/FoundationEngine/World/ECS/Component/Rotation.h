#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct Rotation
	{
		SC_SERIALIZE_FIELD()
		Float x_;

		SC_SERIALIZE_FIELD()
		Float y_;

		SC_SERIALIZE_FIELD()
		Float z_;

		SC_SERIALIZE_FIELD()
		Float w_;
	};
	REGISTER_COMPONENT(Rotation, "Core", ComponentStorage::Archetype);
}
