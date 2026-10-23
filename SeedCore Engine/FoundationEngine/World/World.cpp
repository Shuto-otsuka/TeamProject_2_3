#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>

#include <FoundationEngine/World/Actor/ActorAccessor.h>

#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/WorldMatrix.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>

#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>

#include <FoundationEngine/Coroutine/CoroutineSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Destroys every actor, so their behaviours receive OnDestroy while the
	* services they may use still exist.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全 actor を破棄する。ビヘイビアは、使うかもしれないサービスがまだ
	* 存在する間に OnDestroy を受け取る。
	*/
	World::~World()
	{
		/// [EN] Destroying a root moves its children to the roots, so taking the first root until none are left covers every actor.
		/// [JP] ルートを破棄するとその子がルートへ移るので、ルートが無くなるまで先頭を取り続ければ全 actor を破棄できる。
		while (actors_.Root() != EntityID{})
		{
			DestroyActor(WorldAccessor::Make(*this, actors_.Root()));
		}
	}

	/**
	* [EN]
	* Creates an actor named name with persistentID, or with a new ID when
	* persistentID is 0; CreateActor(name) and loaded scenes both come here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name という名前の actor を persistentID で作る。persistentID が 0 なら
	* 新しい ID を使う。CreateActor(name) も、読み込んだシーンもここを通る。
	*/
	Actor World::CreateActor(String name, Uint32 persistentID)
	{
		Entity entity = entities_.Create();
		actors_.Add(entity.GetID(), persistentID);

		AddComponent(entity, Name{ name });
		AddComponent(entity, Position{ 0.0f,0.0f,0.0f });
		AddComponent(entity, Rotation{ 0.0f,0.0f,0.0f,1.0f });
		AddComponent(entity, Scale{ 1.0f,1.0f,1.0f });
		AddComponent(entity, WorldMatrix{});
		AddComponent(entity, Active{ true });
		AddComponent(entity, Bounds{});

		return ActorAccessor::Make(*this, entity);
	}

	/**
	* [EN]
	* Creates an actor named name as the last scene root, with the
	* components every actor has: Name, Position, Rotation, Scale,
	* WorldMatrix, Active and Bounds.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name という名前の actor をシーンのルートの末尾に作り、全 actor が持つ
	* コンポーネント（Name、Position、Rotation、Scale、WorldMatrix、Active、
	* Bounds）を付ける。
	*/
	Actor World::CreateActor(String name)
	{
		return CreateActor(name, 0);
	}

	/**
	* [EN]
	* Destroys actor: its children move to the scene roots, its behaviours
	* receive OnDestroy, and its components and entity are released.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor を破棄する。子はシーンのルートへ移り、ビヘイビアは OnDestroy を
	* 受け取り、コンポーネントとエンティティは解放される。
	*/
	void World::DestroyActor(Actor actor)
	{
		EntityID entityID = ActorAccessor::GetEntity(actor).GetID();
		ActorRecord* record = actors_.Record(entityID);
		if (!record)
		{
			return;
		}

		/// [EN] Each Attach takes the current first child away, so this repeats until none are left.
		/// [JP] Attach のたびに今の先頭の子が抜けるので、子が無くなるまで繰り返す。
		while (record->child_ != EntityID{})
		{
			actors_.Attach(record->child_, EntityID{});
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since one OnDestroy may remove another behaviour.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。ある OnDestroy が別のビヘイビアを外しうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : entities_.Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		for (ComponentID id : behaviours)
		{
			if (void* data = sparseSets_.Data(entityID, id))
			{
				id->hooks_->upcast_(data)->DispatchDestroy();
			}
		}

		CoroutineSystem::Cancel(entityID);

		/// [EN] The records are dropped before the entity, since the registries look entities up by generation and would no longer find it.
		/// [JP] エンティティより先にレコードを外す。レジストリは世代でエンティティを引くので、後では見つからなくなる。
		sparseSets_.Clear(entityID);
		archetypes_.Clear(entityID);
		actors_.Remove(entityID);
		entities_.Destroy(entityID);
	}

	/**
	* [EN]
	* Returns the first actor, in storage order, whose Name is name, or an
	* invalid Actor if there is none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 格納の順で最初に見つかった、Name が name の actor を返す。無ければ
	* 無効な Actor。
	*/
	Actor World::GetActor(const String& name)
	{
		ComponentID nameID = ComponentRegistry::GetComponentID<Name>();
		Size bit = ComponentRegistry::GetID(nameID);

		for (Archetype* archetype : ArchetypeRegistry::Archetypes())
		{
			const Bitset& signature = archetype->Signature();
			if (bit >= signature.size() || !signature.test(bit))
			{
				continue;
			}

			std::span<Chunk* const> chunks = archetypes_.Chunks(archetype);
			if (chunks.empty())
			{
				continue;
			}

			const DynamicArray<ComponentID>& layout = archetype->Layout();
			Size column = static_cast<Size>(std::ranges::find(layout, nameID) - layout.begin());

			/// [EN] Names sit packed in each chunk's column, so this reads memory in order; interned Strings compare by address, so each check is one comparison.
			/// [JP] 名前は各チャンクの列に詰まって並ぶので、メモリを順に読むだけになる。intern された String はアドレスで比べるので、1件の確認は比較1回で済む。
			for (Chunk* chunk : chunks)
			{
				const Name* names = reinterpret_cast<const Name*>(chunk->Data(column));
				Uint32 count = chunk->EntityCount();
				for (Uint32 row = 0;row < count;++row)
				{
					if (names[row].name_ == name)
					{
						EntityID entityID = chunk->EntityAt(row);
						return ActorAccessor::Make(*this, Entity(Handle<Entity>{entityID.index_, entityID.generation_}));
					}
				}
			}
		}

		return Actor();
	}

	/**
	* [EN]
	* Returns the actor identified by entityID, such as an ID a raycast or
	* an overlap reported, or an invalid Actor if entityID is stale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID が識別する actor を返す。レイキャストやオーバーラップが
	* 返した ID などに使う。entityID が古ければ無効な Actor。
	*/
	Actor World::GetActor(EntityID entityID)
	{
		return WorldAccessor::Make(*this, entityID);
	}

	/**
	* [EN]
	* Returns the actor holding persistentID, the ID an actor reference
	* field keeps through save and load, or an invalid Actor if none does.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* persistentID を持つ actor を返す。persistentID は、actor を参照する
	* フィールドがセーブとロードを通して持つ ID。誰も持っていなければ
	* 無効な Actor。
	*/
	Actor World::GetActor(Uint32 persistentID)
	{
		return WorldAccessor::Find(*this, persistentID);
	}

	/**
	* [EN]
	* Returns every actor, in hierarchy order (each actor before its
	* children, siblings in order). The array is a copy taken at the call,
	* so destroying actors while walking it does not cut the walk short.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全ての actor を、ヒエラルキーの順（親が子より先、兄弟は並び順）で
	* 返す。配列は呼んだ時点の写しなので、たどっている途中で actor を
	* 破棄しても走査が途中で切れない。
	*/
	DynamicArray<Actor> World::Actors()
	{
		/// [EN] Root and Next walk the hierarchy depth-first; every actor is collected before the caller sees any, so the caller may change the hierarchy freely.
		/// [JP] Root と Next はヒエラルキーを深さ優先でたどる。呼び出し側に渡す前に全ての actor を集め終えるので、呼び出し側は階層を自由に変えてよい。
		DynamicArray<Actor> actors;
		for (EntityID entityID = actors_.Root();entityID != EntityID{};entityID = actors_.Next(entityID))
		{
			actors.push_back(GetActor(entityID));
		}
		return actors;
	}

	/**
	* [EN]
	* Default-constructs component id for entity in the storage its
	* metadata selects and returns it; returns the existing one if entity
	* already has it, or nullptr if entity is stale or id is unregistered.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* metadata が選ぶストレージで、entity にコンポーネント id をデフォルト
	* 構築して返す。既に持っていればそれを返し、entity が古いか id が
	* 登録解除済みなら nullptr。
	*/
	void* World::AddComponent(Entity entity, ComponentID id)
	{
		if (!id || !id->metadata_)
		{
			return nullptr;
		}

		if (id->metadata_->storage_ == ComponentStorage::Archetype)
		{
			return archetypes_.Add(entity.GetID(), id);
		}
		return sparseSets_.Add(entity.GetID(), id);
	}

	/**
	* [EN]
	* Returns entity's component id, or nullptr if it has none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entity のコンポーネント id を返す。持っていなければ nullptr。
	*/
	void* World::GetComponent(Entity entity, ComponentID id)const
	{
		if (!id || !id->metadata_)
		{
			return nullptr;
		}

		if (id->metadata_->storage_ == ComponentStorage::Archetype)
		{
			return archetypes_.Data(entity.GetID(), id);
		}
		return sparseSets_.Data(entity.GetID(), id);
	}

	/**
	* [EN]
	* Returns whether entity has component id.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entity がコンポーネント id を持っているかどうかを返す。
	*/
	Bool World::HasComponent(Entity entity, ComponentID id)const
	{
		if (!id || !id->metadata_)
		{
			return false;
		}

		if (id->metadata_->storage_ == ComponentStorage::Archetype)
		{
			return archetypes_.Has(entity.GetID(), id);
		}
		return sparseSets_.Has(entity.GetID(), id);
	}

	/**
	* [EN]
	* Destroys entity's component id, if it has one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entity がコンポーネント id を持っていれば破棄する。
	*/
	void World::RemoveComponent(Entity entity, ComponentID id)
	{
		if (!id || !id->metadata_)
		{
			return;
		}

		if (id->metadata_->storage_ == ComponentStorage::Archetype)
		{
			archetypes_.Remove(entity.GetID(), id);
		}
		sparseSets_.Remove(entity.GetID(), id);
	}
}