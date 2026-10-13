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
	struct LinearVelocity
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

		/// [EN] Whether a physics body owns this velocity; MoveSystem then leaves Position to the body.
		/// [JP] この速度を物理ボディが管理しているか。そのとき MoveSystem は Position をボディに任せる。
		Bool simulated_ = false;
	};
	REGISTER_COMPONENT(LinearVelocity, "Core", ComponentStorage::Archetype);

	/**
	* [EN]
	* Component holding an entity's angular velocity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの角速度を保持するコンポーネント。
	*/
	struct AngularVelocity
	{
		/// [EN] Angular velocity about the X axis.
		/// [JP] X 軸まわりの角速度。
		SC_SERIALIZE_FIELD()
		Float x_;

		/// [EN] Angular velocity about the Y axis.
		/// [JP] Y 軸まわりの角速度。
		SC_SERIALIZE_FIELD()
		Float y_;

		/// [EN] Angular velocity about the Z axis.
		/// [JP] Z 軸まわりの角速度。
		SC_SERIALIZE_FIELD()
		Float z_;

		/// [EN] Whether a physics body owns this angular velocity; Rotation is then turned by the body.
		/// [JP] この角速度を物理ボディが管理しているか。そのとき Rotation はボディが回す。
		Bool simulated_ = false;
	};
	REGISTER_COMPONENT(AngularVelocity, "Core", ComponentStorage::Archetype);
}
