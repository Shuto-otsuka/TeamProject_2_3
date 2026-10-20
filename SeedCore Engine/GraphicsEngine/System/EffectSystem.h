#pragma once

namespace SeedCore
{
	class World;

	class EffectSystem
	{
	public:
		void Update(World& world, Float deltaTime);
	};
}