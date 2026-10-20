#pragma once

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Built-in system integrating every actor's Position by LinearVelocity
	* and Rotation by AngularVelocity, by deltaTime each frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全 actor の Position を LinearVelocity で、Rotation を AngularVelocity
	* で、毎フレーム deltaTime 分だけ積分する組み込みシステム。
	*/
	class MoveSystem
	{
	public:
		/**
		* [EN]
		* Advances every active actor by deltaTime: LinearVelocity is added
		* onto Position, and AngularVelocity turns Rotation about its axis.
		* Inactive actors, and actors whose velocity is owned by a physics
		* body, keep their Position and Rotation unchanged.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アクティブな全 actor を deltaTime 分だけ進める。LinearVelocity は
		* Position へ加算し、AngularVelocity はその軸まわりに Rotation を回す。
		* 非アクティブな actor と、速度を物理ボディが管理している actor の
		* Position と Rotation は変えない。
		*/
		void Execute(World& world, Float deltaTime);
	};
}
