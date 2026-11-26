#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct PrefabRecord
	{
		EntityID entity_;

		Uint32 assetID_ = 0;

		Bool fromPrefab_ = false;
	};

	class SEEDCORE_API PrefabRegistry :public NonTransferable
	{
	public:
		void Add(EntityID entityID, Uint32 assetID, Bool fromPrefab);

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
		DynamicArray<PrefabRecord> records_;
	};
}