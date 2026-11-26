#pragma once
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	class PrefabSystem;

	class SEEDCORE_API Prefab
	{
	public:
		static Actor Spawn(Uint32 assetID, Actor parent = {});

		static Actor Spawn(const String& name, Actor parent = {});

	private:
		friend class PrefabSystem;

	private:
		static PrefabSystem* system_;
	};
}
