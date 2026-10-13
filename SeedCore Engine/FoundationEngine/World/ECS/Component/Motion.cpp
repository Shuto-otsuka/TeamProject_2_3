#include <FoundationEngine/World/ECS/Component/Motion.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns the linear velocity as a Vector3.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 速度を Vector3 で返す。
	*/
	Vector3 Motion::Vector(const LinearVelocity& velocity)
	{
		return Vector3(velocity.x_, velocity.y_, velocity.z_);
	}

	/**
	* [EN]
	* Writes value into the linear velocity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を速度に書き込む。
	*/
	void Motion::Vector(LinearVelocity& velocity, const Vector3& value)
	{
		velocity.x_ = value.x;
		velocity.y_ = value.y;
		velocity.z_ = value.z;
	}

	/**
	* [EN]
	* Returns the angular velocity as a Vector3 whose direction is the
	* axis and whose length is the speed of the turn.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 角速度を Vector3 で返す。向きが回転軸、長さが回転の速さ。
	*/
	Vector3 Motion::Vector(const AngularVelocity& velocity)
	{
		return Vector3(velocity.x_, velocity.y_, velocity.z_);
	}

	/**
	* [EN]
	* Writes value into the angular velocity; its direction is the axis
	* and its length the speed of the turn.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を角速度に書き込む。向きが回転軸、長さが回転の速さ。
	*/
	void Motion::Vector(AngularVelocity& velocity, const Vector3& value)
	{
		velocity.x_ = value.x;
		velocity.y_ = value.y;
		velocity.z_ = value.z;
	}
}
