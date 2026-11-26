#pragma once

namespace SeedCore
{
	class World;
	class Actor;

	class ViewportPicking
	{
	public:
		static Actor Pick(World& world, const Vector3& rayOrigin, const Vector3& rayDirection);

	private:
		static Bool RayIntersectsLocalAABB(const Vector3& rayOriginLocal, const Vector3& rayDirectionLocal, const Vector3& boundsMin, const Vector3& boundsMax, Float& outT);
	};
}
