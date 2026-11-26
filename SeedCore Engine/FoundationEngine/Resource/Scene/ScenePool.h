#pragma once
#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class ResourceCache;

	class ScenePool
	{
	public:
		Handle<SceneBlueprint> Load(Uint32 assetID, ResourceCache& cache);

		SceneBlueprint* Get(Handle<SceneBlueprint> handle);

		SceneBlueprint& GetScratch();

	private:
		StablePool<SceneBlueprint> pool_;

		FlatMap<Uint32, Handle<SceneBlueprint>> loaded_;

		SceneBlueprint scratch_;
	};
}
