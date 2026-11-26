#include <FoundationEngine/World/Command/CommandBuffer.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/Prefab/PrefabPool.h>
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	EntityID CommandBuffer::SpawnPrefab(Uint32 assetID, const Vector3& position, EntityID parent)
	{
		EntityID provisional = EntityID{ provisionalCount_++, ProvisionalGeneration };

		Command command;
		command.kind_ = Kind::SpawnPrefab;
		command.target_ = provisional;
		command.parent_ = parent;
		command.assetID_ = assetID;
		command.position_ = position;
		commands_.push_back(command);

		return provisional;
	}

	void CommandBuffer::DestroyEntity(EntityID id)
	{
		Command command;
		command.kind_ = Kind::DestroyEntity;
		command.target_ = id;
		commands_.push_back(command);
	}

	void CommandBuffer::AddComponent(EntityID id, ComponentID componentID, const void* source)
	{
		const ComponentMetadata& meta = ComponentRegistry::Get(componentID);

		Size cellCount = (meta.size_ + sizeof(BlobCell) - 1) / sizeof(BlobCell);
		Size cellOffset = blob_.size();
		blob_.resize(blob_.size() + cellCount);
		meta.copy_(blob_.data() + cellOffset, source);

		Command command;
		command.kind_ = Kind::AddComponent;
		command.target_ = id;
		command.componentID_ = componentID;
		command.dataOffset_ = cellOffset;
		commands_.push_back(command);
	}

	void CommandBuffer::RemoveComponent(EntityID id, ComponentID componentID)
	{
		Command command;
		command.kind_ = Kind::RemoveComponent;
		command.target_ = id;
		command.componentID_ = componentID;
		commands_.push_back(command);
	}

	Bool CommandBuffer::Empty()const
	{
		return commands_.empty();
	}

	void CommandBuffer::Flush(World& world, ResourceCache& cache)
	{
		for (const Command& command : commands_)
		{
			EntityID target = command.target_;
			if (Provisional(target))
			{
				auto it = resolved_.find(target.index_);
				target = (it != resolved_.end()) ? it->second : EntityID{};
			}

			switch (command.kind_)
			{
			case Kind::SpawnPrefab:
			{
				EntityID parentID = command.parent_;
				if (Provisional(parentID))
				{
					auto it = resolved_.find(parentID.index_);
					parentID = (it != resolved_.end()) ? it->second : EntityID{};
				}

				Actor parent = (parentID != EntityID{}) ? WorldAccessor::Make(world, parentID) : Actor();

				Handle<PrefabBlueprint> handle = cache.Prefabs().Load(command.assetID_, cache);
				PrefabBlueprint* prefab = cache.Prefabs().Get(handle);
				if (prefab == nullptr)
				{
					break;
				}

				Actor spawned = prefab->Instantiate(world, cache, parent, command.assetID_);
				if (!spawned)
				{
					break;
				}

				resolved_[command.target_.index_] = ActorAccessor::GetEntity(spawned).GetID();

				Position* position = spawned.GetComponent<Position>();
				if (position != nullptr)
				{
					Transform::Vector(*position, command.position_);
				}
				break;
			}
			case Kind::DestroyEntity:
			{
				if (target == EntityID{})
				{
					break;
				}

				Actor actor = WorldAccessor::Make(world, target);
				if (actor)
				{
					world.DestroyActor(actor);
				}
				break;
			}
			case Kind::AddComponent:
			{
				if (target == EntityID{})
				{
					break;
				}

				const ComponentMetadata& meta = ComponentRegistry::Get(command.componentID_);
				Entity entity(Handle<Entity>{ target.index_, target.generation_ });

				world.AddComponent(entity, command.componentID_);

				void* destination = world.GetComponent(entity, command.componentID_);
				if (destination != nullptr)
				{
					void* recorded = blob_.data() + command.dataOffset_;
					meta.destruct_(destination);
					meta.copy_(destination, recorded);
				}
				break;
			}
			case Kind::RemoveComponent:
			{
				if (target == EntityID{})
				{
					break;
				}

				world.RemoveComponent(Entity(Handle<Entity>{ target.index_, target.generation_ }), command.componentID_);
				break;
			}
			}
		}

		for (const Command& command : commands_)
		{
			if (command.kind_ == Kind::AddComponent)
			{
				const ComponentMetadata& meta = ComponentRegistry::Get(command.componentID_);
				meta.destruct_(blob_.data() + command.dataOffset_);
			}
		}

		commands_.clear();
		blob_.clear();
		provisionalCount_ = 0;
	}

	EntityID CommandBuffer::Resolved(EntityID provisional)const
	{
		if (!Provisional(provisional))
		{
			return EntityID{};
		}

		auto it = resolved_.find(provisional.index_);
		return (it != resolved_.end()) ? it->second : EntityID{};
	}

	void CommandBuffer::Clear()
	{
		commands_.clear();
		blob_.clear();
		resolved_.clear();
		provisionalCount_ = 0;
	}

	Bool CommandBuffer::Provisional(EntityID id)
	{
		return id.generation_ == ProvisionalGeneration;
	}
}
