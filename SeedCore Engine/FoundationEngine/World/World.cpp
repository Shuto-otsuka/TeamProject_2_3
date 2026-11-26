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
	World::~World()
	{
		while (actors_.Root() != EntityID{})
		{
			DestroyActor(WorldAccessor::Make(*this, actors_.Root()));
		}
	}

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

	Actor World::CreateActor(String name)
	{
		return CreateActor(name, 0);
	}

	void World::DestroyActor(Actor actor)
	{
		EntityID entityID = ActorAccessor::GetEntity(actor).GetID();
		ActorRecord* record = actors_.Record(entityID);
		if (!record)
		{
			return;
		}

		while (record->child_ != EntityID{})
		{
			actors_.Attach(record->child_, EntityID{});
		}

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

		sparseSets_.Clear(entityID);
		archetypes_.Clear(entityID);
		actors_.Remove(entityID);
		entities_.Destroy(entityID);
	}

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

	Actor World::GetActor(EntityID entityID)
	{
		return WorldAccessor::Make(*this, entityID);
	}

	Actor World::GetActor(Uint32 persistentID)
	{
		return WorldAccessor::Find(*this, persistentID);
	}

	DynamicArray<Actor> World::Actors()
	{
		DynamicArray<Actor> actors;
		for (EntityID entityID = actors_.Root();entityID != EntityID{};entityID = actors_.Next(entityID))
		{
			actors.push_back(GetActor(entityID));
		}
		return actors;
	}

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