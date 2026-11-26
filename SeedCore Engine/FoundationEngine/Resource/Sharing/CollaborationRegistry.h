#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct CollaborationRecord
	{
		EntityID entity_;

		String id_;
	};

	class SEEDCORE_API CollaborationRegistry :public NonTransferable
	{
	public:
		void Add(EntityID entityID, String id);

		template<typename Self>
		auto Record(this Self& self, EntityID entityID) -> decltype(&self.records_[0])
		{
			if (entityID.index_ >= self.records_.size() || self.records_[entityID.index_].entity_ != entityID)
			{
				return nullptr;
			}
			return &self.records_[entityID.index_];
		}

	private:
		DynamicArray<CollaborationRecord> records_;
	};
}