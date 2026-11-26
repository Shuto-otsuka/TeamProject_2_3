#pragma once

namespace SeedCore
{

	class World;
	class AnimationResource;
	class ModelResource;

	class AnimationSystem
	{
	public:
		void Execute(World& world, AnimationResource& animationResource, ModelResource& modelResource);
	};
}
