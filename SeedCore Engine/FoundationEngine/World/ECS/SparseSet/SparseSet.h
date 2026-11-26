#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	class SEEDCORE_API SparseSet :public NonTransferable
	{
	public:
		SparseSet(ComponentID id);

		~SparseSet();

		Uint32 Add(EntityID entityID);

		EntityID Remove(Uint32 row);

		void* Data(Uint32 row)const;

		Uint32 Epoch()const;

		EntityID EntityAt(Uint32 row)const;

		Uint32 EntityCount()const;

	private:
		SC_CONSTEX Size pageSize_ = 16 * 1024;

		ComponentID id_ = nullptr;

		Uint32 epoch_ = 0;

		Size elementSize_ = 0;

		Size elementAlignment_ = 0;

		Size pageCapacity_ = 0;

		DynamicArray<Uint8*> pages_;

		Size lastPageUsed_ = 0;

		DynamicArray<void*> freeSlots_;

		DynamicArray<EntityID> entityIDs_;

		DynamicArray<void*> data_;
	};
}