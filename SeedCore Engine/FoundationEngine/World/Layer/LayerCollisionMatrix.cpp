#include <FoundationEngine/World/Layer/LayerCollisionMatrix.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>

namespace SeedCore
{
	Bool LayerCollisionMatrix::GetCollide(Size layerA, Size layerB)
	{
		if (layerA >= LayerRegistry::LayerCount || layerB >= LayerRegistry::LayerCount)
		{
			return true;
		}

		return Entries()[TriangleIndex(layerA, layerB)];
	}

	void LayerCollisionMatrix::SetCollide(Size layerA, Size layerB, Bool collide)
	{
		if (layerA >= LayerRegistry::LayerCount || layerB >= LayerRegistry::LayerCount)
		{
			return;
		}

		Entries()[TriangleIndex(layerA, layerB)] = collide;
	}

	DynamicArray<Bool>& LayerCollisionMatrix::Entries()
	{
		static DynamicArray<Bool> entries(LayerRegistry::LayerCount * (LayerRegistry::LayerCount + 1) / 2, true);
		return entries;
	}

	Size LayerCollisionMatrix::TriangleIndex(Size layerA, Size layerB)
	{
		Size lo = Min(layerA, layerB);
		Size hi = Max(layerA, layerB);
		return hi * (hi + 1) / 2 + lo;
	}
}
