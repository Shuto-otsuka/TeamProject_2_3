#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	/**
	* [EN]
	* Builds a handle to the actor identified by entity within world.
	* Does not create anything; World::CreateActor does that.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world 内で entity が識別する actor へのハンドルを作る。何も生成
	* しない。生成するのは World::CreateActor。
	*/
	Actor ActorAccessor::Make(World& world, Entity entity)
	{
		return Actor(world, entity);
	}

	/**
	* [EN]
	* Returns the entity actor wraps.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor が包むエンティティを返す。
	*/
	Entity ActorAccessor::GetEntity(Actor actor)
	{
		return actor.entity_;
	}
}