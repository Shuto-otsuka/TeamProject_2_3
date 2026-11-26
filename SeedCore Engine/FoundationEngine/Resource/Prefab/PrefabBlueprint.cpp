#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Json/JsonArchive.h>

namespace SeedCore
{
	void PrefabBlueprint::Capture(Actor root)
	{
		nodes_.clear();
		BlueprintNode::Capture(root, nodes_);
	}

	Actor PrefabBlueprint::Instantiate(World& world, ResourceCache& cache, Actor parent, Uint32 sourceAssetID)const
	{
		DynamicArray<Actor> instantiated = BlueprintNode::Instantiate(world, cache, nodes_, parent, true);
		if (instantiated.empty() || !instantiated[0])
		{
			return Actor();
		}

		WorldAccessor::Services(world).Add<PrefabRegistry>()->Add(ActorAccessor::GetEntity(instantiated[0]).GetID(), sourceAssetID, true);
		return instantiated[0];
	}

	Bool PrefabBlueprint::Write(const std::filesystem::path& path)
	{
		JsonOutputArchive archive;
		Save(archive);
		return archive.Write(String(path.string()));
	}

	Bool PrefabBlueprint::Read(const std::filesystem::path& path)
	{
		JsonInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			SC_LOG_ERROR("Prefabの読み込みに失敗しました ({})", path.string());
			return false;
		}

		Load(archive);

		return true;
	}
}
