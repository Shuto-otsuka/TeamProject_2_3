#pragma once
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	/**
	* [EN]
	* The engine's one way through World's private side: turning entity IDs
	* into actors, creating and finding actors by persistent ID, and
	* reaching the registries and storages a world owns. Queries, snapshots,
	* scene loading, the editor and Actor's own implementation go through
	* here. This header is not part of what scripts include, so it stays out
	* of their view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* World の private な面へ、エンジンが入る唯一の入口。エンティティ ID を
	* actor に変える、永続 ID で actor を作る・探す、ワールドが持つレジストリ
	* とストレージへ届く。Query、スナップショット、シーンの読み込み、
	* エディタ、Actor 自身の実装はここを通る。このヘッダはスクリプトが取り
	* 込むものに含めないので、スクリプトからは見えない。
	*/
	class SEEDCORE_API WorldAccessor
	{
	public:
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
		static Actor Make(World& world, EntityID entityID);

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
		static Actor Create(World& world, String name, Uint32 persistentID);

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
		static Actor Find(World& world, Uint32 persistentID);

		/**
		* [EN]
		* Destroys every actor in world, each through World::DestroyActor so
		* every behaviour receives OnDestroy. Used when a scene is replaced.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world の全ての actor を破棄する。1体ずつ World::DestroyActor を
		* 通すので、全てのビヘイビアが OnDestroy を受け取る。シーンを
		* 入れ替えるときに使う。
		*/
		static void Clear(World& world);

		/**
		* [EN]
		* Returns world's entity registry.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world のエンティティレジストリを返す。
		*/
		static EntityRegistry& Entities(World& world);

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
		static ActorRegistry& Actors(World& world);

		/**
		* [EN]
		* Returns world's archetype storage.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world のアーキタイプストレージを返す。
		*/
		static ArchetypeStorage& Archetypes(World& world);

		/**
		* [EN]
		* Returns world's sparse set storage.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world のスパースセットストレージを返す。
		*/
		static SparseSetStorage& SparseSets(World& world);

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
		static Service& Services(World& world);
	};
}