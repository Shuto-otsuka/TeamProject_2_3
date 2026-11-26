#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	Vector3 Transform::Vector(const Position& position)
	{
		return Vector3(position.x_, position.y_, position.z_);
	}

	void Transform::Vector(Position& position, const Vector3& value)
	{
		position.x_ = value.x;
		position.y_ = value.y;
		position.z_ = value.z;
	}

	Vector3 Transform::Vector(const Scale& scale)
	{
		return Vector3(scale.x_, scale.y_, scale.z_);
	}

	void Transform::Vector(Scale& scale, const Vector3& value)
	{
		scale.x_ = value.x;
		scale.y_ = value.y;
		scale.z_ = value.z;
	}

	Quaternion Transform::Quat(const Rotation& rotation)
	{
		return Quaternion(rotation.x_, rotation.y_, rotation.z_, rotation.w_);
	}

	void Transform::Quat(Rotation& rotation, const Quaternion& value)
	{
		rotation.x_ = value.x;
		rotation.y_ = value.y;
		rotation.z_ = value.z;
		rotation.w_ = value.w;
	}

	Vector3 Transform::Euler(const Rotation& rotation)
	{
		return Quaternion(rotation.x_, rotation.y_, rotation.z_, rotation.w_).ToEuler();
	}

	Vector3 Transform::Degree(const Rotation& rotation)
	{
		Vector3 euler = Quaternion(rotation.x_, rotation.y_, rotation.z_, rotation.w_).ToEuler();
		return Vector3(ToDegrees(euler.x), ToDegrees(euler.y), ToDegrees(euler.z));
	}
}
