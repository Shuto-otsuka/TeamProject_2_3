#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component holding an entity's local-space rotation, in Euler angles.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティのローカル空間回転（オイラー角）を保持するコンポーネント。
	*/
	struct Rotation
	{
		/// [EN] Rotation about the X axis.
		/// [JP] X 軸周りの回転。
		SC_SERIALIZE_FIELD()
		Float x_;

		/// [EN] Rotation about the Y axis.
		/// [JP] Y 軸周りの回転。
		SC_SERIALIZE_FIELD()
		Float y_;

		/// [EN] Rotation about the Z axis.
		/// [JP] Z 軸周りの回転。
		SC_SERIALIZE_FIELD()
		Float z_;

		/**
        * [EN]
        * Returns the rotation as a Vector3.
        *
        * ---------------------------------------------------------------------
        *
        * [JP]
        * 回転を Vector3 として取得する。
        */
		[[nodiscard]] Vector3 Vector()const noexcept
		{
			return Vector3(x_, y_, z_);
		}

		/**
		* [EN]
		* Returns the rotation as a Quaternion.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回転を Quaternion として取得する。
		*/
		[[nodiscard]] Quaternion Quat()const noexcept
		{
			return Quaternion::CreateFromYawPitchRoll(ToRadians(y_), ToRadians(x_), ToRadians(z_));
		}
	};
	REGISTER_COMPONENT(Rotation, "Core", ComponentStorage::Archetype);
}
