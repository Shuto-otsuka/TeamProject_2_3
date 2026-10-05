#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads and writes the Position, Rotation and Scale components as math
	* types. The components stay plain data stored in archetype chunks; the
	* conversions live here instead. Passing only the component reads it,
	* passing a value as well writes it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Position・Rotation・Scale の各コンポーネントを、数学の型として読み書き
	* する。コンポーネントはアーキタイプのチャンクに置くただのデータのまま
	* にし、変換はここに置く。コンポーネントだけを渡すと読み出し、値も渡すと
	* 書き込みになる。
	*/
	class SEEDCORE_API Transform
	{
	public:
		/**
		* [EN]
		* Returns the position as a Vector3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 位置を Vector3 で返す。
		*/
		[[nodiscard]] static Vector3 Vector(const Position& position);

		/**
		* [EN]
		* Writes value into the position.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* value を位置に書き込む。
		*/
		static void Vector(Position& position, const Vector3& value);

		/**
		* [EN]
		* Returns the scale as a Vector3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 拡大縮小を Vector3 で返す。
		*/
		[[nodiscard]] static Vector3 Vector(const Scale& scale);

		/**
		* [EN]
		* Writes value into the scale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* value を拡大縮小に書き込む。
		*/
		static void Vector(Scale& scale, const Vector3& value);

		/**
		* [EN]
		* Returns the rotation as a Quaternion.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回転を Quaternion で返す。
		*/
		[[nodiscard]] static Quaternion Quat(const Rotation& rotation);

		/**
		* [EN]
		* Writes value into the rotation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* value を回転に書き込む。
		*/
		static void Quat(Rotation& rotation, const Quaternion& value);

		/**
		* [EN]
		* Returns the rotation as Euler angles in radians.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回転をオイラー角（ラジアン）で返す。
		*/
		[[nodiscard]] static Vector3 Euler(const Rotation& rotation);

		/**
		* [EN]
		* Returns the rotation as Euler angles in degrees.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回転をオイラー角（度）で返す。
		*/
		[[nodiscard]] static Vector3 Degree(const Rotation& rotation);
	};
}
