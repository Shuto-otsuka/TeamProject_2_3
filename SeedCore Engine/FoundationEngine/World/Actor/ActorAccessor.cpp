#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	Actor ActorAccessor::Make(World& world, Entity entity)
	{
		return Actor(world, entity);
	}

	Entity ActorAccessor::GetEntity(Actor actor)
	{
		return actor.entity_;
	}
}