#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component holding an entity's world-space transform matrix, computed
	* each frame from its local position, rotation and scale and its
	* parent's WorldMatrix. A cache of those values, so it is never saved.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティのワールド空間の変換行列を保持するコンポーネント。ローカルの
	* 位置・回転・スケールと、親の WorldMatrix から毎フレーム計算する。
	* それらから求まるキャッシュなので保存しない。
	*/
	struct WorldMatrix
	{
		/// [EN] The world-space transform matrix.
		/// [JP] ワールド空間の変換行列。
		Matrix matrix_ = Matrix::Identity;
	};
	REGISTER_COMPONENT(WorldMatrix, "Core", ComponentStorage::Archetype);
}