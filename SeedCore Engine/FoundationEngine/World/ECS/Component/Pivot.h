#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component holding the point, in the actor's own local space, that
	* Scale and Rotation are applied around. Position is where that point
	* lands in the parent's space, so the local matrix becomes
	* T(-Pivot) * S * R * T(Position). Children follow the actor's world
	* matrix, so scaling or rotating a parent scales or rotates its whole
	* subtree around this point as well. Every actor has one, starting at
	* the origin.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Scale と Rotation をかける中心の点を、Actor 自身のローカル空間で保持
	* するコンポーネント。Position はその点が親の空間で置かれる位置になる
	* ので、ローカル行列は T(-Pivot) * S * R * T(Position) になる。子は
	* Actor のワールド行列に従うので、親を拡縮・回転すると子孫ごとこの点を
	* 中心に拡縮・回転する。全ての Actor が持ち、初期値は原点。
	*/
	struct Pivot
	{
		/// [EN] X coordinate of the pivot point.
		/// [JP] 基準点の X 座標。
		SC_SERIALIZE_FIELD()
		Float x_;

		/// [EN] Y coordinate of the pivot point.
		/// [JP] 基準点の Y 座標。
		SC_SERIALIZE_FIELD()
		Float y_;

		/// [EN] Z coordinate of the pivot point.
		/// [JP] 基準点の Z 座標。
		SC_SERIALIZE_FIELD()
		Float z_;
	};
	REGISTER_COMPONENT(Pivot, "Core", ComponentStorage::Archetype);
}
