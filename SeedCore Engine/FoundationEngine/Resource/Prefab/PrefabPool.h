#pragma once
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class ResourceCache;

	class SEEDCORE_API PrefabPool
	{
	public:
		Handle<PrefabBlueprint> Load(Uint32 assetID, ResourceCache& cache);

		PrefabBlueprint* Get(Handle<PrefabBlueprint> handle);

	private:
		StablePool<PrefabBlueprint> pool_;

		FlatMap<Uint32, Handle<PrefabBlueprint>> loaded_;
	};
}
