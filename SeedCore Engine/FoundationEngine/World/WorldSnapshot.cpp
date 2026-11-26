#include <FoundationEngine/World/WorldSnapshot.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	void WorldSnapshot::Capture(World& world)
	{
		Clear();

		ActorRegistry& actors = WorldAccessor::Actors(world);
		ArchetypeStorage& archetypes = WorldAccessor::Archetypes(world);
		EntityRegistry& entities = WorldAccessor::Entities(world);

		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			ActorData actorData;
			actorData.entity_ = current;
			Entity entity = Entity(Handle<Entity>{ current.index_, current.generation_ });

			for (ComponentID id : archetypes.Layout(current))
			{
				void* source = world.GetComponent(entity, id);
				if (!source)
				{
					continue;
				}

				const ComponentMetadata& meta = ComponentRegistry::Get(id);
				ComponentData componentData;
				componentData.id_ = id;
				componentData.data_.resize(meta.size_);
				meta.copy_(componentData.data_.data(), source);
				actorData.components_.push_back(std::move(componentData));
			}

			for (const SparseSetLocation& location : entities.Record(current)->sparseSet_)
			{
				const ComponentMetadata& meta = ComponentRegistry::Get(location.id_);
				ComponentData componentData;
				componentData.id_ = location.id_;
				componentData.data_.resize(meta.size_);
				meta.copy_(componentData.data_.data(), location.data_);
				actorData.components_.push_back(std::move(componentData));

				if (location.id_->hooks_ && location.id_->hooks_->upcast_)
				{
					actorData.componentBaseIDs_.push_back(location.id_);
				}
			}

			actors_.push_back(std::move(actorData));
		}

		hasData_ = true;
	}

	void WorldSnapshot::Restore(World& world)
	{
		if (!hasData_)
		{
			return;
		}

		ActorRegistry& actors = WorldAccessor::Actors(world);
		EntityRegistry& entities = WorldAccessor::Entities(world);

		DynamicArray<EntityID> captured;

		for (const ActorData& actorData : actors_)
		{
			if (actorData.entity_.index_ >= captured.size())
			{
				captured.resize(actorData.entity_.index_ + 1);
			}
			captured[actorData.entity_.index_] = actorData.entity_;

			if (!entities.Exists(actorData.entity_))
			{
				continue;
			}
			Entity entity = Entity(Handle<Entity>{ actorData.entity_.index_, actorData.entity_.generation_ });

			for (ComponentID id : actorData.componentBaseIDs_)
			{
				if (void* data = world.GetComponent(entity, id))
				{
					id->hooks_->upcast_(data)->DispatchDestroy();
				}
			}

			for (const ComponentData& componentData : actorData.components_)
			{
				void* dest = world.GetComponent(entity, componentData.id_);
				if (!dest)
				{
					continue;
				}

				const ComponentMetadata& meta = ComponentRegistry::Get(componentData.id_);
				meta.destruct_(dest);
				meta.copy_(dest, componentData.data_.data());
			}

			for (ComponentID id : actorData.componentBaseIDs_)
			{
				if (void* data = world.GetComponent(entity, id))
				{
					id->hooks_->upcast_(data)->ResetLifecycle();
				}
			}
		}

		DynamicArray<Actor> created;
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			if (current.index_ >= captured.size() || captured[current.index_] != current)
			{
				created.push_back(WorldAccessor::Make(world, current));
			}
		}

		for (Actor actor : created)
		{
			world.DestroyActor(actor);
		}

		Clear();
	}

	Bool WorldSnapshot::HasData()const
	{
		return hasData_;
	}

	void WorldSnapshot::Clear()
	{
		for (ActorData& actorData : actors_)
		{
			for (ComponentData& componentData : actorData.components_)
			{
				const ComponentMetadata& meta = ComponentRegistry::Get(componentData.id_);
				meta.destruct_(componentData.data_.data());
			}
		}
		actors_.clear();
		hasData_ = false;
	}
}
