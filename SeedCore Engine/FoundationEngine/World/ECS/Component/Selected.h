#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [EN]
	* Tag marking an entity the editor currently has selected. The editor
	* adds and removes it from SelectionContext; renderers read the set of
	* selected entities from it instead of being handed a list each frame.
	* It is editor state, so it is never saved and never shown in the
	* Inspector. Stored in a sparse set so selecting does not move the
	* entity's archetype row.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタで選択中のエンティティに付くタグ。エディタが SelectionContext
	* から付け外しし、描画側は毎フレーム一覧を受け取る代わりに、このタグから
	* 選択中のものを読む。エディタの状態なので保存せず、Inspector にも出さない。
	* 選択でアーキタイプの行が移動しないよう、スパースセットに置く。
	*/
	struct Selected
	{
		/// No Code
	};
	REGISTER_COMPONENT(Selected, "Core", ComponentStorage::SparseSet);
}