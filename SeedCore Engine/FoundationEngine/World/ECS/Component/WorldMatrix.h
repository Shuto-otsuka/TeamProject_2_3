#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct WorldMatrix
	{
		Matrix matrix_ = Matrix::Identity;
	};
	REGISTER_COMPONENT(WorldMatrix, "Core", ComponentStorage::Archetype);
}