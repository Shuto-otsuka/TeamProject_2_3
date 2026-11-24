#pragma once

namespace SeedCore
{
	class EffectResource;
	class EffekseerResource;
	class World;

	class EffectSystem
	{
	public:
		void Update(EffectResource& effectResource, EffekseerResource& effekseerResource, World& world, Float deltaTime);
	};
}