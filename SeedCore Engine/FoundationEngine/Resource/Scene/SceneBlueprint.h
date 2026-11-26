#pragma once
#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class Actor;
	class SceneSystem;

	class SEEDCORE_API SceneBlueprint
	{
	public:
		void Capture(World& world);

		void Clear();

		DynamicArray<Actor> Instantiate(World& world, ResourceCache& cache)const;

		Bool Write(const std::filesystem::path& path);

		Bool Read(const std::filesystem::path& path);

	public:
		NODISCARD const DynamicArray<BlueprintNode>& Nodes()const;

		NODISCARD const String& Name()const;

	public:
		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("nodes", nodes_);
		}

		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("nodes", nodes_);
		}

	private:
		friend class SceneSystem;

	private:
		DynamicArray<BlueprintNode> nodes_;

		String name_;
	};
}
