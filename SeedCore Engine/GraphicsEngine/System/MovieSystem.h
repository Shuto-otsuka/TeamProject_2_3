#pragma once
#include <FoundationEngine/Prelude.h>

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
