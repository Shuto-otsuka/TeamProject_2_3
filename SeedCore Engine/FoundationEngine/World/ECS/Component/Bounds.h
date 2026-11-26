#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct Bounds
	{
		Vector3 center_ = { 0.0f, 0.0f, 0.0f };
		Vector3 extent_ = { 0.5f, 0.5f, 0.5f };
	};
	REGISTER_COMPONENT(Bounds, "Geometry", ComponentStorage::Archetype);
}
