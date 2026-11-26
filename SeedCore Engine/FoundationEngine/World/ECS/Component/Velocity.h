#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct LinearVelocity
	{
		SC_SERIALIZE_FIELD()
		Float x_;

		SC_SERIALIZE_FIELD()
		Float y_;

		SC_SERIALIZE_FIELD()
		Float z_;
	};
	REGISTER_COMPONENT(LinearVelocity, "Core", ComponentStorage::SparseSet);

	struct AngularVelocity
	{
		SC_SERIALIZE_FIELD()
		Float x_;

		SC_SERIALIZE_FIELD()
		Float y_;

		SC_SERIALIZE_FIELD()
		Float z_;
	};
	REGISTER_COMPONENT(AngularVelocity, "Core", ComponentStorage::SparseSet);
}
