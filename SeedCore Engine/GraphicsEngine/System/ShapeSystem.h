#pragma once
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class World;

	class ShapeSystem
	{
	public:
		void Update(World& world, Bool visible, Float aspectRatio);

	public:
		NODISCARD std::span<const ShapeDesc> Shapes()const;

	private:
		SC_CONSTEX Float frustumLength_ = 10.0f;

		SC_CONSTEX Float directionalRadius_ = 0.5f;

		SC_CONSTEX Float directionalRayLength_ = 2.0f;

		SC_CONSTEX Uint directionalRayCount_ = 8;

		SC_CONSTEX Float worldArrowHeadLength_ = 0.3f;

		SC_CONSTEX Float canvasArrowHeadLength_ = 15.0f;

		SC_CONSTEX Float queryHitRadius_ = 0.05f;

		SC_CONSTEX Float queryNormalLength_ = 0.3f;

		SC_CONSTEX Float queryHitPixels_ = 5.0f;

		SC_CONSTEX Float queryNormalPixels_ = 30.0f;

		DynamicArray<ShapeDesc> shapes_;
	};
}
