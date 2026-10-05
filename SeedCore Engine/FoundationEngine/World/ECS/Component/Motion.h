#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads and writes the Velocity component as a math type. The component
	* stays plain data stored in archetype chunks; the conversion lives here
	* instead. Passing only the component reads it, passing a value as well
	* writes it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Velocity コンポーネントを数学の型として読み書きする。コンポーネントは
	* アーキタイプのチャンクに置くただのデータのままにし、変換はここに置く。
	* コンポーネントだけを渡すと読み出し、値も渡すと書き込みになる。
	*/
	class SEEDCORE_API Motion
	{
	public:
		/**
		* [EN]
		* Returns the velocity as a Vector3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 速度を Vector3 で返す。
		*/
		[[nodiscard]] static Vector3 Vector(const Velocity& velocity);

		/**
		* [EN]
		* Writes value into the velocity.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* value を速度に書き込む。
		*/
		static void Vector(Velocity& velocity, const Vector3& value);
	};
}
