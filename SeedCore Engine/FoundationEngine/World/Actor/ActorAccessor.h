#pragma once
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	class SEEDCORE_API ActorAccessor
	{
	public:
		static Actor Make(World& world, Entity entity);

		static Entity GetEntity(Actor actor);
	};
}