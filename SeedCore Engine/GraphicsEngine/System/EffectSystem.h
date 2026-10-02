#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	class World;

	class EffectSystem
	{
	public:
		void Update(World& world, Float deltaTime);
	};
}