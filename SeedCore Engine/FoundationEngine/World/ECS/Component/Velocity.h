#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component holding an entity's linear velocity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの線形速度を保持するコンポーネント。
	*/
	struct Velocity
	{
		/// [EN] Velocity along the X axis.
		/// [JP] X 軸方向の速度。
		SC_SERIALIZE_FIELD()
		Float x_;

		/// [EN] Velocity along the Y axis.
		/// [JP] Y 軸方向の速度。
		SC_SERIALIZE_FIELD()
		Float y_;

		/// [EN] Velocity along the Z axis.
		/// [JP] Z 軸方向の速度。
		SC_SERIALIZE_FIELD()
		Float z_;

		/**
		* [EN]
		* Returns the velocity as a Vector3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 速度を Vector3 として取得する。
		*/
		[[nodiscard]] Vector3 Vector()const noexcept
		{
			return Vector3(x_, y_, z_);
		}
	};
	REGISTER_COMPONENT(Velocity, "Core", ComponentStorage::Archetype);
}
