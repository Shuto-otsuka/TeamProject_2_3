#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>

namespace SeedCore
{
	void PrefabRegistry::Add(EntityID entityID, Uint32 assetID, Bool fromPrefab)
	{
		if (entityID.index_ >= records_.size())
		{
			records_.resize(entityID.index_ + 1);
		}
		records_[entityID.index_] = PrefabRecord{ entityID,assetID,fromPrefab };
	}
}