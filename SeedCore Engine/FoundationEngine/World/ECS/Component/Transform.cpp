#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns the position as a Vector3.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 位置を Vector3 で返す。
	*/
	Vector3 Transform::Vector(const Position& position)
	{
		return Vector3(position.x_, position.y_, position.z_);
	}

	/**
	* [EN]
	* Writes value into the position.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を位置に書き込む。
	*/
	void Transform::Vector(Position& position, const Vector3& value)
	{
		position.x_ = value.x;
		position.y_ = value.y;
		position.z_ = value.z;
	}

	/**
	* [EN]
	* Returns the scale as a Vector3.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 拡大縮小を Vector3 で返す。
	*/
	Vector3 Transform::Vector(const Scale& scale)
	{
		return Vector3(scale.x_, scale.y_, scale.z_);
	}

	/**
	* [EN]
	* Writes value into the scale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を拡大縮小に書き込む。
	*/
	void Transform::Vector(Scale& scale, const Vector3& value)
	{
		scale.x_ = value.x;
		scale.y_ = value.y;
		scale.z_ = value.z;
	}

	/**
	* [EN]
	* Returns the rotation as a Quaternion.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 回転を Quaternion で返す。
	*/
	Quaternion Transform::Quat(const Rotation& rotation)
	{
		return Quaternion(rotation.x_, rotation.y_, rotation.z_, rotation.w_);
	}

	/**
	* [EN]
	* Writes value into the rotation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を回転に書き込む。
	*/
	void Transform::Quat(Rotation& rotation, const Quaternion& value)
	{
		rotation.x_ = value.x;
		rotation.y_ = value.y;
		rotation.z_ = value.z;
		rotation.w_ = value.w;
	}

	/**
	* [EN]
	* Returns the rotation as Euler angles in radians.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 回転をオイラー角（ラジアン）で返す。
	*/
	Vector3 Transform::Euler(const Rotation& rotation)
	{
		return Quaternion(rotation.x_, rotation.y_, rotation.z_, rotation.w_).ToEuler();
	}

	/**
	* [EN]
	* Returns the rotation as Euler angles in degrees.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 回転をオイラー角（度）で返す。
	*/
	Vector3 Transform::Degree(const Rotation& rotation)
	{
		Vector3 euler = Quaternion(rotation.x_, rotation.y_, rotation.z_, rotation.w_).ToEuler();
		return Vector3(ToDegrees(euler.x), ToDegrees(euler.y), ToDegrees(euler.z));
	}
}
