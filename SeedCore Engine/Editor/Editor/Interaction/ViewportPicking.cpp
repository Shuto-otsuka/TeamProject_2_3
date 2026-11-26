#include <Editor/Editor/Interaction/ViewportPicking.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>

namespace SeedCore
{
	Actor ViewportPicking::Pick(World& world, const Vector3& rayOrigin, const Vector3& rayDirection)
	{
		ComponentID boundsComponentID = ComponentRegistry::GetComponentID<Bounds>();
		if (!boundsComponentID)
		{
			return Actor();
		}

		Actor closestActor;
		Float closestDistance = FLT_MAX;

		DynamicArray<EntityID> boundsEntities;
		Query<Read<Bounds>>(world).ForEach([&](EntityID entityID, const Bounds&) { boundsEntities.push_back(entityID); });
		for (EntityID entityID : boundsEntities)
		{
			Actor actor = WorldAccessor::Make(world, entityID);
			if (!actor || !actor.Active())
			{
				continue;
			}

			const Bounds* bounds = actor.GetComponent<Bounds>();
			if (!bounds)
			{
				continue;
			}

			Matrix worldMatrix = actor.WorldMatrix();
			Matrix inverseWorld = worldMatrix.Invert();

			Vector3 localOrigin = Vector3::TransformPosition(rayOrigin, inverseWorld);
			Vector3 localDirection = Vector3::TransformDirection(rayDirection, inverseWorld);

			Vector3 boundsMin = bounds->center_ - bounds->extent_;
			Vector3 boundsMax = bounds->center_ + bounds->extent_;

			Float localT;
			if (!RayIntersectsLocalAABB(localOrigin, localDirection, boundsMin, boundsMax, localT))
			{
				continue;
			}

			Vector3 localHitPoint = localOrigin + localDirection * localT;
			Vector3 worldHitPoint = Vector3::TransformPosition(localHitPoint, worldMatrix);
			Float worldDistance = Vector3::Distance(rayOrigin, worldHitPoint);

			if (worldDistance < closestDistance)
			{
				closestDistance = worldDistance;
				closestActor = actor;
			}
		}

		return closestActor;
	}

	Bool ViewportPicking::RayIntersectsLocalAABB(const Vector3& rayOriginLocal, const Vector3& rayDirectionLocal, const Vector3& boundsMin, const Vector3& boundsMax, Float& outT)
	{
		Float origin[3] = { rayOriginLocal.x, rayOriginLocal.y, rayOriginLocal.z };
		Float direction[3] = { rayDirectionLocal.x, rayDirectionLocal.y, rayDirectionLocal.z };
		Float boundsMinArray[3] = { boundsMin.x, boundsMin.y, boundsMin.z };
		Float boundsMaxArray[3] = { boundsMax.x, boundsMax.y, boundsMax.z };

		Float tMin = -FLT_MAX;
		Float tMax = FLT_MAX;

		for (Int axis = 0; axis < 3; axis++)
		{
			if (std::abs(direction[axis]) < 1e-8f)
			{
				if (origin[axis] < boundsMinArray[axis] || origin[axis] > boundsMaxArray[axis])
				{
					return false;
				}
				continue;
			}

			Float inverseDirection = 1.0f / direction[axis];
			Float t0 = (boundsMinArray[axis] - origin[axis]) * inverseDirection;
			Float t1 = (boundsMaxArray[axis] - origin[axis]) * inverseDirection;
			if (t0 > t1)
			{
				std::swap(t0, t1);
			}

			tMin = Max(tMin, t0);
			tMax = Min(tMax, t1);
			if (tMin > tMax)
			{
				return false;
			}
		}

		if (tMax < 0.0f)
		{
			return false;
		}

		outT = (tMin >= 0.0f) ? tMin : tMax;
		return true;
	}
}
