#pragma once

namespace SeedCore
{
	class World;
	class MaterialResource;
	class ModelResource;

	class MaterialSystem
	{
	public:
		void Execute(World& world, MaterialResource& materialResource, ModelResource& modelResource);
	};
}
