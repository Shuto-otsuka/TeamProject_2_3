#pragma once
#include <FoundationEngine/World/ECS/Component/Velocity.h>

namespace SeedCore
{
	class SEEDCORE_API Motion
	{
	public:
		NODISCARD static Vector3 Vector(const LinearVelocity& velocity);

		static void Vector(LinearVelocity& velocity, const Vector3& value);

		NODISCARD static Vector3 Vector(const AngularVelocity& velocity);

		static void Vector(AngularVelocity& velocity, const Vector3& value);
	};
}
