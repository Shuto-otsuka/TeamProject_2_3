#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component holding an entity's local-space rotation as a normalized
	* quaternion. Euler angles are derived only at human-facing boundaries.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティのローカル空間回転を正規化済みクォータニオンとして保持する
	* コンポーネント。オイラー角は人が扱う境界でのみ導出する。
	*/
	struct Rotation
	{
		/// [EN] Quaternion X component.
		/// [JP] クォータニオンの X 成分。
		SC_SERIALIZE_FIELD()
		Float x_;

		/// [EN] Quaternion Y component.
		/// [JP] クォータニオンの Y 成分。
		SC_SERIALIZE_FIELD()
		Float y_;

		/// [EN] Quaternion Z component.
		/// [JP] クォータニオンの Z 成分。
		SC_SERIALIZE_FIELD()
		Float z_;

		/// [EN] Quaternion scalar component; one for the identity rotation.
		/// [JP] クォータニオンのスカラー成分。恒等回転では 1 となる。
		SC_SERIALIZE_FIELD()
		Float w_;
	};
	REGISTER_COMPONENT(Rotation, "Core", ComponentStorage::Archetype);
}
