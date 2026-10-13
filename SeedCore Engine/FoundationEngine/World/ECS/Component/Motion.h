#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads and writes the LinearVelocity and AngularVelocity components as
	* a math type. The components stay plain data stored in archetype
	* chunks; the conversion lives here instead. Passing only the component
	* reads it, passing a value as well writes it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* LinearVelocity と AngularVelocity コンポーネントを数学の型として読み
	* 書きする。コンポーネントはアーキタイプのチャンクに置くただのデータの
	* ままにし、変換はここに置く。コンポーネントだけを渡すと読み出し、値も
	* 渡すと書き込みになる。
	*/
	class SEEDCORE_API Motion
	{
	public:
		/**
		* [EN]
		* Returns the linear velocity as a Vector3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 速度を Vector3 で返す。
		*/
		[[nodiscard]] static Vector3 Vector(const LinearVelocity& velocity);

		/**
		* [EN]
		* Writes value into the linear velocity.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* value を速度に書き込む。
		*/
		static void Vector(LinearVelocity& velocity, const Vector3& value);

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
		[[nodiscard]] static Vector3 Vector(const AngularVelocity& velocity);

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
		static void Vector(AngularVelocity& velocity, const Vector3& value);
	};
}
