#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component holding an entity's local-space position.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティのローカル空間位置を保持するコンポーネント。
	*/
	struct Position
	{
		/// [EN] X coordinate.
		/// [JP] X 座標。
		SC_SERIALIZE_FIELD()
		Float x_;

		/// [EN] Y coordinate.
		/// [JP] Y 座標。
		SC_SERIALIZE_FIELD()
		Float y_;

		/// [EN] Z coordinate.
		/// [JP] Z 座標。
		SC_SERIALIZE_FIELD()
		Float z_;

		/**
        * [EN]
        * Returns the position as a Vector3.
        *
        * ---------------------------------------------------------------------
        *
        * [JP]
        * 位置を Vector3 として取得する。
        */
		[[nodiscard]] Vector3 Vector()const noexcept
		{
			return Vector3(x_, y_, z_);
		}
	};
	REGISTER_COMPONENT(Position, "Core", ComponentStorage::Archetype);
}
