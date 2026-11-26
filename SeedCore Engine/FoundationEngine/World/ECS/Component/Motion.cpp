#include <FoundationEngine/World/ECS/Component/Motion.h>

namespace SeedCore
{
	Vector3 Motion::Vector(const LinearVelocity& velocity)
	{
		return Vector3(velocity.x_, velocity.y_, velocity.z_);
	}

	void Motion::Vector(LinearVelocity& velocity, const Vector3& value)
	{
		velocity.x_ = value.x;
		velocity.y_ = value.y;
		velocity.z_ = value.z;
	}

	Vector3 Motion::Vector(const AngularVelocity& velocity)
	{
		return Vector3(velocity.x_, velocity.y_, velocity.z_);
	}

	void Motion::Vector(AngularVelocity& velocity, const Vector3& value)
	{
		velocity.x_ = value.x;
		velocity.y_ = value.y;
		velocity.z_ = value.z;
	}
}
