#pragma once

namespace SeedCore
{
	class World;
	class MovieResource;

	class MovieSystem
	{
	public:
		void Update(MovieResource& movieResource, World& world);
	};
}
