#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>
#include <FoundationEngine/Resource/Sharing/CollaborationRegistry.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/Tag/TagRegistry.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/WorldMatrix.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>

namespace SeedCore
{
	void BlueprintNode::Apply(Actor actor)const
	{
		if (!actor)
		{
			return;
		}

		World& world = actor.GetWorld();
		EntityID entityID = ActorAccessor::GetEntity(actor).GetID();

		if (nestedPrefabAssetID_ == 0)
		{
			const DynamicArray<ComponentID>& layout = WorldAccessor::Archetypes(world).Layout(entityID);
			DynamicArray<ComponentID> owned(layout.begin(), layout.end());
			for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
			{
				owned.push_back(location.id_);
			}

			for (ComponentID id : owned)
			{
				if (Builtin(id) || std::ranges::contains(components_, ComponentRegistry::Name(id), &BlueprintComponent::componentName_))
				{
					continue;
				}
				actor.RemoveComponent(id);
			}

			for (const BlueprintComponent& component : components_)
			{
				ComponentID id = ComponentRegistry::GetComponentID(component.componentName_);
				if (!id)
				{
					UnknownComponent::Keep(actor, component);
					continue;
				}

				void* componentData = actor.AddComponent(id);
				if (componentData)
				{
					component.Apply(componentData);
				}
			}
		}

		Name* name = actor.GetComponent<Name>();
		if (name)
		{
			name->name_ = name_;
		}

		Position* position = actor.GetComponent<Position>();
		if (position)
		{
			Transform::Vector(*position, position_);
		}

		Rotation* rotation = actor.GetComponent<Rotation>();
		if (rotation)
		{
			Transform::Quat(*rotation, rotation_);
		}

		Scale* scale = actor.GetComponent<Scale>();
		if (scale)
		{
			Transform::Vector(*scale, scale_);
		}

		actor.Active(active_);
		actor.Layer(layerName_);

		ActorRecord* record = WorldAccessor::Actors(world).Record(entityID);
		record->tags_ = 0;
		record->extraTags_ = nullptr;
		for (const String& tag : tags_)
		{
			actor.AddTag(tag);
		}
	}

	void BlueprintNode::Capture(Actor root, DynamicArray<BlueprintNode>& nodes)
	{
		Capture(root, -1, nodes);
	}

	DynamicArray<Actor> BlueprintNode::Instantiate(World& world, ResourceCache& cache, const DynamicArray<BlueprintNode>& nodes, Actor parent, Bool fromPrefab)
	{
		DynamicArray<Actor> actors;
		actors.reserve(nodes.size());

		for (const BlueprintNode& node : nodes)
		{
			Actor nodeParent = parent;
			if (node.parentIndex_ >= 0 && static_cast<Size>(node.parentIndex_) < actors.size())
			{
				nodeParent = actors[node.parentIndex_];
			}
			actors.push_back(node.Instantiate(world, cache, nodeParent, fromPrefab));
		}

		return actors;
	}

	Actor BlueprintNode::Instantiate(World& world, ResourceCache& cache, Actor parent, Bool fromPrefab)const
	{
		Actor actor;
		Service& services = WorldAccessor::Services(world);

		if (nestedPrefabAssetID_ != 0)
		{
			Handle<PrefabBlueprint> nestedHandle = cache.Prefabs().Load(nestedPrefabAssetID_, cache);
			PrefabBlueprint* nestedPrefab = cache.Prefabs().Get(nestedHandle);
			if (nestedPrefab)
			{
				actor = nestedPrefab->Instantiate(world, cache, parent, nestedPrefabAssetID_);
			}

			if (!actor)
			{
				return Actor();
			}
		}
		else
		{
			actor = WorldAccessor::Create(world, name_, persistentId_);

			if (fromPrefab)
			{
				services.Add<PrefabRegistry>()->Add(ActorAccessor::GetEntity(actor).GetID(), 0, true);
			}
		}

		if (!fromPrefab && collaborationId_ != String{})
		{
			services.Add<CollaborationRegistry>()->Add(ActorAccessor::GetEntity(actor).GetID(), collaborationId_);
		}

		Apply(actor);

		if (parent)
		{
			actor.Parent(parent);
		}

		return actor;
	}

	Bool BlueprintNode::Builtin(ComponentID id)
	{
		return id == ComponentRegistry::GetComponentID<Name>() || id == ComponentRegistry::GetComponentID<Position>() || id == ComponentRegistry::GetComponentID<Rotation>() || id == ComponentRegistry::GetComponentID<Scale>() || id == ComponentRegistry::GetComponentID<Active>() || id == ComponentRegistry::GetComponentID<WorldMatrix>() || id == ComponentRegistry::GetComponentID<Bounds>() || id == ComponentRegistry::GetComponentID<LinearVelocity>() || id == ComponentRegistry::GetComponentID<AngularVelocity>() || id == ComponentRegistry::GetComponentID<Selected>();
	}

	Int BlueprintNode::Capture(Actor actor, Int parentIndex, DynamicArray<BlueprintNode>& nodes)
	{
		World& world = actor.GetWorld();
		EntityID entityID = ActorAccessor::GetEntity(actor).GetID();
		ActorRegistry& actors = WorldAccessor::Actors(world);
		const ActorRecord* record = actors.Record(entityID);
		Service& services = WorldAccessor::Services(world);

		BlueprintNode node;

		const Name* name = actor.GetComponent<Name>();
		node.name_ = name ? name->name_ : String("Actor");

		TagRegistry::ForEach(*record, [&node](const String& tag)
			{
				node.tags_.push_back(tag);
			});
		node.layerName_ = actor.LayerName();
		node.active_ = actor.Active();
		node.persistentId_ = record->persistentID_;

		const CollaborationRegistry* collaborations = services.Get<CollaborationRegistry>();
		const CollaborationRecord* collaboration = collaborations ? collaborations->Record(entityID) : nullptr;
		node.collaborationId_ = collaboration ? collaboration->id_ : String{};

		const Position* position = actor.GetComponent<Position>();
		if (position)
		{
			node.position_ = Vector3(position->x_, position->y_, position->z_);
		}

		const Rotation* rotation = actor.GetComponent<Rotation>();
		if (rotation)
		{
			node.rotation_ = Transform::Quat(*rotation);
		}

		const Scale* scale = actor.GetComponent<Scale>();
		if (scale)
		{
			node.scale_ = Vector3(scale->x_, scale->y_, scale->z_);
		}

		node.parentIndex_ = parentIndex;

		const PrefabRegistry* prefabs = services.Get<PrefabRegistry>();
		const PrefabRecord* prefab = prefabs ? prefabs->Record(entityID) : nullptr;
		Uint32 prefabID = prefab ? prefab->assetID_ : 0;
		Bool isNestedInstance = (parentIndex != -1) && (prefabID != 0);

		if (isNestedInstance)
		{
			node.nestedPrefabAssetID_ = prefabID;
		}
		else
		{
			for (ComponentID id : WorldAccessor::Archetypes(world).Layout(entityID))
			{
				if (Builtin(id))
				{
					continue;
				}
				node.components_.push_back(BlueprintComponent::Capture(ComponentRegistry::Name(id), actor.GetComponent(id)));
			}

			ComponentID unknownID = ComponentRegistry::GetComponentID<UnknownComponent>();
			for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
			{
				if (location.id_ == unknownID)
				{
					const UnknownComponent* unknown = static_cast<const UnknownComponent*>(location.data_);
					node.components_.insert(node.components_.end(), unknown->components_.begin(), unknown->components_.end());
					continue;
				}
				node.components_.push_back(BlueprintComponent::Capture(ComponentRegistry::Name(location.id_), location.data_));
			}
		}

		Int index = static_cast<Int>(nodes.size());
		nodes.push_back(std::move(node));

		if (!isNestedInstance)
		{
			for (EntityID child = record->child_; child != EntityID{}; child = actors.Record(child)->nextSibling_)
			{
				Capture(WorldAccessor::Make(world, child), index, nodes);
			}
		}

		return index;
	}
}
