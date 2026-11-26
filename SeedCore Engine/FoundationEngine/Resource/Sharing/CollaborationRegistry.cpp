#include <FoundationEngine/Resource/Sharing/CollaborationRegistry.h>

namespace SeedCore
{
	void CollaborationRegistry::Add(EntityID entityID, String id)
	{
		if (entityID.index_ >= records_.size())
		{
			records_.resize(entityID.index_ + 1);
		}
		records_[entityID.index_] = CollaborationRecord{ entityID,id };
	}
}