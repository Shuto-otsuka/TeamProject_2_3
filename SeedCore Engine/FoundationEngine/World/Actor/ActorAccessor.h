#pragma once
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	/**
	* [EN]
	* The engine's one way through Actor's private side: building an Actor
	* from an entity, and reading the entity an Actor wraps. Actor keeps both
	* private so scripts only ever see actors they were handed. This header
	* is not part of what scripts include, so it stays out of their view.
	* Both functions are defined here, so they inline into the per-entity
	* loops that call them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor の private な面へ、エンジンが入る唯一の入口: エンティティから
	* Actor を作ることと、Actor が包むエンティティを読むこと。Actor は
	* どちらも private にして、スクリプトには渡された actor しか見えない
	* ようにしている。このヘッダはスクリプトが取り込むものに含めないので、
	* スクリプトからは見えない。どちらの関数もここで定義するので、呼び出す
	* エンティティごとのループの中へ展開される。
	*/
	class SEEDCORE_API ActorAccessor
	{
	public:
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
		static Actor Make(World& world, Entity entity);

		/**
		* [EN]
		* Returns the entity actor wraps.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor が包むエンティティを返す。
		*/
		static Entity GetEntity(Actor actor);
	};
}