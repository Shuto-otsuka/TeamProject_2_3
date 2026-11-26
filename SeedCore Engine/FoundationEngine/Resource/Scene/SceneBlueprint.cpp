#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Json/JsonArchive.h>

namespace SeedCore
{
	void SceneBlueprint::Capture(World& world)
	{
		nodes_.clear();

		ActorRegistry& actors = WorldAccessor::Actors(world);
		for (EntityID root = actors.Root(); root != EntityID{}; root = actors.Record(root)->nextSibling_)
		{
			BlueprintNode::Capture(WorldAccessor::Make(world, root), nodes_);
		}
	}

	void SceneBlueprint::Clear()
	{
		nodes_.clear();
		nodes_.shrink_to_fit();
	}

	DynamicArray<Actor> SceneBlueprint::Instantiate(World& world, ResourceCache& cache)const
	{
		return BlueprintNode::Instantiate(world, cache, nodes_, Actor(), false);
	}

	Bool SceneBlueprint::Write(const std::filesystem::path& path)
	{
		JsonOutputArchive archive;
		Save(archive);
		return archive.Write(String(path.string()));
	}

	Bool SceneBlueprint::Read(const std::filesystem::path& path)
	{
		JsonInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			SC_LOG_ERROR("シーンの読み込みに失敗しました ({})", path.string());
			return false;
		}

		Load(archive);
		name_ = String(path.stem().string());

		return true;
	}

	const DynamicArray<BlueprintNode>& SceneBlueprint::Nodes()const
	{
		return nodes_;
	}

	const String& SceneBlueprint::Name()const
	{
		return name_;
	}
}
