#pragma once
#include <FoundationEngine/World/Layer/LayerRegistry.h>

namespace SeedCore
{
	class SEEDCORE_API LayerCollisionMatrix
	{
	public:
		static Bool GetCollide(Size layerA, Size layerB);

		static void SetCollide(Size layerA, Size layerB, Bool collide);

		template<typename Archive>
		static void Save(Archive& archive)
		{
			archive.Field("collisionEntries", Entries());
		}

		template<typename Archive>
		static void Load(Archive& archive)
		{
			DynamicArray<Bool> loaded;
			if (!archive.TryField("collisionEntries", loaded))
			{
				return;
			}

			Size expectedCount = LayerRegistry::LayerCount * (LayerRegistry::LayerCount + 1) / 2;
			if (loaded.size() != expectedCount)
			{
				return;
			}

			Entries() = std::move(loaded);
		}

	private:
		static DynamicArray<Bool>& Entries();

		static Size TriangleIndex(Size layerA, Size layerB);
	};
}
