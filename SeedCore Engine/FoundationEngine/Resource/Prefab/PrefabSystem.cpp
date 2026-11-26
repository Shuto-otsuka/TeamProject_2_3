#include <FoundationEngine/Resource/Prefab/PrefabSystem.h>
#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabPool.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	Bool PrefabSystem::Initialize(World& world, ResourceCache& cache)
	{
		world_ = &world;
		resource_ = &cache;
		Prefab::system_ = this;
		return true;
	}

	PrefabSystem::~PrefabSystem()
	{
		if (Prefab::system_ == this)
		{
			Prefab::system_ = nullptr;
		}
	}

	Actor PrefabSystem::Spawn(Uint32 assetID, Actor parent)
	{
		PrefabPool& pool = resource_->Prefabs();
		PrefabBlueprint* prefab = pool.Get(pool.Load(assetID, *resource_));
		if (prefab == nullptr)
		{
			return Actor();
		}

		return prefab->Instantiate(*world_, *resource_, parent, assetID);
	}

	Actor PrefabSystem::Spawn(const String& name, Actor parent)
	{
		Uint32 assetID = resource_->Find(name);
		if (assetID == 0)
		{
			SC_LOG_WARNING("Prefab::Spawn: プレハブ \"{}\" が見つかりません。", name.c_str());
			return Actor();
		}

		return Spawn(assetID, parent);
	}

	Bool PrefabSystem::Save(Actor root, const std::filesystem::path& path)
	{
		PrefabBlueprint prefab;
		prefab.Capture(root);

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		return prefab.Write(path);
	}

	std::filesystem::path PrefabSystem::SaveToDirectory(Actor root, const std::filesystem::path& directory)
	{
		PrefabBlueprint prefab;
		prefab.Capture(root);

		std::filesystem::create_directories(directory);

		const Name* name = root.GetComponent<Name>();
		std::string baseName = (name && !name->name_.str().empty()) ? name->name_.str() : "Actor";

		std::filesystem::path prefabPath = directory / (baseName + ".prefab");
		Int suffix = 1;
		while (std::filesystem::exists(prefabPath))
		{
			prefabPath = directory / (baseName + "(" + std::to_string(suffix++) + ").prefab");
		}

		if (!prefab.Write(prefabPath))
		{
			return {};
		}

		return prefabPath;
	}
}
