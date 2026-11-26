#pragma once
#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	class Actor;
	class World;
	class ResourceCache;

	struct BlueprintNode
	{
		String name_;

		DynamicArray<String> tags_;

		String layerName_;

		Bool active_ = true;

		Vector3 position_ = Vector3(0.0f, 0.0f, 0.0f);

		Quaternion rotation_ = Quaternion::Identity;

		Vector3 scale_ = Vector3(1.0f, 1.0f, 1.0f);

		DynamicArray<BlueprintComponent> components_;

		Int parentIndex_ = -1;

		Uint32 persistentId_ = 0;

		String collaborationId_;

		Uint32 nestedPrefabAssetID_ = 0;

		DynamicArray<BlueprintField> overrides_;

		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("name", name_);
			archive.Field("tags", tags_);
			archive.Field("layer", layerName_);
			archive.Field("active", active_);
			archive.Field("position", position_);
			archive.Field("rotation", rotation_);
			archive.Field("scale", scale_);
			archive.Field("components", components_);
			archive.Field("parentIndex", parentIndex_);
			archive.Field("nestedPrefabAssetID", nestedPrefabAssetID_);
			archive.Field("overrides", overrides_);
			archive.Field("persistentId", persistentId_);
			archive.Field("collaborationId", collaborationId_);
		}

		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("name", name_);
			archive.TryField("tags", tags_);
			archive.TryField("layer", layerName_);
			archive.TryField("active", active_);
			archive.TryField("position", position_);
			archive.TryField("rotation", rotation_);
			archive.TryField("scale", scale_);
			archive.TryField("components", components_);
			archive.TryField("parentIndex", parentIndex_);
			archive.TryField("nestedPrefabAssetID", nestedPrefabAssetID_);
			archive.TryField("overrides", overrides_);
			archive.TryField("persistentId", persistentId_);
			archive.TryField("collaborationId", collaborationId_);
		}

		SEEDCORE_API void Apply(Actor actor)const;

		SEEDCORE_API static void Capture(Actor root, DynamicArray<BlueprintNode>& nodes);

		SEEDCORE_API static DynamicArray<Actor> Instantiate(World& world, ResourceCache& cache, const DynamicArray<BlueprintNode>& nodes, Actor parent, Bool fromPrefab);

		SEEDCORE_API Actor Instantiate(World& world, ResourceCache& cache, Actor parent, Bool fromPrefab)const;

	private:
		static Bool Builtin(ComponentID id);

		static Int Capture(Actor actor, Int parentIndex, DynamicArray<BlueprintNode>& nodes);
	};
}