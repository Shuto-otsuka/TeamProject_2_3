#pragma once
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>

namespace SeedCore
{
	class Actor;
	class World;
	class ResourceCache;

	using PrefabComponentField = BlueprintField;

	using PrefabComponent = BlueprintComponent;

	using PrefabNode = BlueprintNode;

	class SEEDCORE_API PrefabBlueprint
	{
	public:
		void Capture(Actor root);

		Actor Instantiate(World& world, ResourceCache& cache, Actor parent = {}, Uint32 sourceAssetID = 0)const;

		Bool Write(const std::filesystem::path& path);

		Bool Read(const std::filesystem::path& path);

		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("nodes", nodes_);
			archive.Field("basePrefabAssetID", basePrefabAssetID_);
		}

		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("nodes", nodes_);
			archive.TryField("basePrefabAssetID", basePrefabAssetID_);
		}

	private:
		DynamicArray<PrefabNode> nodes_;

		Uint32 basePrefabAssetID_ = 0;
	};
}
