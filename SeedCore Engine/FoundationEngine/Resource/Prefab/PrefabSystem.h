#pragma once
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/Utility/NonTransferable.h>

namespace SeedCore
{
	class World;
	class ResourceCache;

	class SEEDCORE_API PrefabSystem :public NonTransferable
	{
	public:
		PrefabSystem() = default;

		~PrefabSystem();

		Bool Initialize(World& world, ResourceCache& cache);

	public:
		Actor Spawn(Uint32 assetID, Actor parent = {});

		Actor Spawn(const String& name, Actor parent = {});

	public:
		static Bool Save(Actor root, const std::filesystem::path& path);

		static std::filesystem::path SaveToDirectory(Actor root, const std::filesystem::path& directory);

	private:
		World* world_ = nullptr;

		ResourceCache* resource_ = nullptr;
	};
}
