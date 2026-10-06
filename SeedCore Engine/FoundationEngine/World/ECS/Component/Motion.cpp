#include <FoundationEngine/World/ECS/Component/Motion.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns the velocity as a Vector3.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 速度を Vector3 で返す。
	*/
	Vector3 Motion::Vector(const Velocity& velocity)
	{
		return Vector3(velocity.x_, velocity.y_, velocity.z_);
	}

	/**
	* [EN]
	* Writes value into the velocity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を速度に書き込む。
	*/
	void Motion::Vector(Velocity& velocity, const Vector3& value)
	{
		velocity.x_ = value.x;
		velocity.y_ = value.y;
		velocity.z_ = value.z;
	}
}
