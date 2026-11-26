#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/Resource/Prefab/PrefabSystem.h>

namespace SeedCore
{
	PrefabSystem* Prefab::system_ = nullptr;

	Actor Prefab::Spawn(Uint32 assetID, Actor parent)
	{
		if (!system_)
		{
			return Actor();
		}

		return system_->Spawn(assetID, parent);
	}

	Actor Prefab::Spawn(const String& name, Actor parent)
	{
		if (!system_)
		{
			return Actor();
		}

		return system_->Spawn(name, parent);
	}
}
