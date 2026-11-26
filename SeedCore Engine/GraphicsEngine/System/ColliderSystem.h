#pragma once
#include <FoundationEngine/Interop/ShapeInstance.h>

namespace SeedCore
{
	class World;

	class ColliderSystem
	{
	public:
		void Update(World& world);

	public:
		NODISCARD std::span<const ShapeDesc> Colliders()const;

	private:
		DynamicArray<ShapeDesc> colliders_;
	};
}
