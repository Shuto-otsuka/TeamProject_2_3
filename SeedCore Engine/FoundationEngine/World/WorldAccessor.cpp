#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	/**
	* [EN]
	* Builds a handle to the actor identified by entityID within world, or
	* an invalid Actor if entityID is stale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world 内で entityID が識別する actor へのハンドルを作る。entityID が
	* 古ければ無効な Actor。
	*/
	Actor WorldAccessor::Make(World& world, EntityID entityID)
	{
		if (!world.entities_.Exists(entityID))
		{
			return Actor();
		}
		return ActorAccessor::Make(world, Entity(Handle<Entity>{entityID.index_, entityID.generation_}));
	}

	/**
	* [EN]
	* Creates an actor named name with persistentID, for loading a saved
	* scene or prefab.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name という名前の actor を persistentID で作る。保存したシーンや
	* プレハブを読み込むときに使う。
	*/
	Actor WorldAccessor::Create(World& world, String name, Uint32 persistentID)
	{
		return world.CreateActor(name, persistentID);
	}

	/**
	* [EN]
	* Returns the actor holding persistentID in world, or an invalid Actor
	* if none does.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world で persistentID を持つ actor を返す。誰も持っていなければ無効な
	* Actor。
	*/
	Actor WorldAccessor::Find(World& world, Uint32 persistentID)
	{
		return Make(world, world.actors_.Find(persistentID));
	}

	/**
	* [EN]
	* Destroys every actor in world, each through World::DestroyActor so
	* every behaviour receives OnDestroy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world の全ての actor を破棄する。1体ずつ World::DestroyActor を
	* 通すので、全てのビヘイビアが OnDestroy を受け取る。
	*/
	void WorldAccessor::Clear(World& world)
	{
		/// [EN] DestroyActor moves the destroyed actor's children to the end of the scene roots, so destroying the first root until none is left reaches every actor, with one move per child.
		/// [JP] DestroyActor は破棄した actor の子をシーンのルートの末尾へ付け替えるので、先頭のルートを無くなるまで破棄し続ければ全ての actor に届く。付け替えは子1体につき1回。
		ActorRegistry& actors = world.actors_;
		for (EntityID root = actors.Root();root != EntityID{};root = actors.Root())
		{
			world.DestroyActor(Make(world, root));
		}
	}

	/**
	* [EN]
	* Returns world's entity registry.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world のエンティティレジストリを返す。
	*/
	EntityRegistry& WorldAccessor::Entities(World& world)
	{
		return world.entities_;
	}

	/**
	* [EN]
	* Returns world's actor registry, for walking the hierarchy and reading
	* records.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world の actor レジストリを返す。階層をたどるときや、レコードを読む
	* ときに使う。
	*/
	ActorRegistry& WorldAccessor::Actors(World& world)
	{
		return world.actors_;
	}

	/**
	* [EN]
	* Returns world's archetype storage.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world のアーキタイプストレージを返す。
	*/
	ArchetypeStorage& WorldAccessor::Archetypes(World& world)
	{
		return world.archetypes_;
	}

	/**
	* [EN]
	* Returns world's sparse set storage.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world のスパースセットストレージを返す。
	*/
	SparseSetStorage& WorldAccessor::SparseSets(World& world)
	{
		return world.sparseSets_;
	}

	/**
	* [EN]
	* Returns world's services, for code that reaches a service by its
	* type's name rather than through World::GetService<T>.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world のサービスを返す。World::GetService<T> ではなく、型の名前で
	* サービスを引くコードが使う。
	*/
	Service& WorldAccessor::Services(World& world)
	{
		return world.service_;
	}
}