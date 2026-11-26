#pragma once
#include <FoundationEngine/World/ECS/SparseSet/SparseSet.h>
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	class SEEDCORE_API SparseSetStorage :public NonTransferable
	{
	public:
		SparseSetStorage(EntityRegistry& entities);

		void* Add(EntityID entityID, ComponentID id);

		void Remove(EntityID entityID, ComponentID id);

		void Clear(EntityID entityID);

		void* Data(EntityID entityID, ComponentID id)const;

		Bool Has(EntityID entityID, ComponentID id)const;

		SparseSet* Set(ComponentID id)const;

		Uint32 Version()const;

	private:
		EntityRegistry& entities_;

		DynamicArray<ResourcePtr<SparseSet>> sets_;

		Uint32 version_ = 0;
	};
}