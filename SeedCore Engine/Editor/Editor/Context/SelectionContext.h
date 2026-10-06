#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	/**
	* [EN]
	* The actors selected in the editor, in the order they were selected.
	* The last one is the primary selection, which single-target panels
	* such as the Inspector show.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタで選択中のアクター。選んだ順に並ぶ。最後の1つが主な選択で、
	* Inspector のように1つだけを対象にするパネルはこれを表示する。
	*/
	struct SelectionContext
	{
		/// [EN] Selected actors in selection order; the last is the primary selection.
		/// [JP] 選択中のアクター。選んだ順に並び、最後が主な選択。
		DynamicArray<Actor> actors_;

		/**
		* [EN]
		* The primary selection: the actor selected last, or an invalid
		* Actor when nothing is selected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 主な選択。最後に選んだアクター。何も選んでいなければ無効な Actor。
		*/
		Actor Primary()const;

		/**
		* [EN]
		* Makes actor the only selection; an invalid actor clears it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor だけを選択した状態にする。無効な actor なら選択を空にする。
		*/
		void Select(Actor actor);

		/**
		* [EN]
		* Removes actor from the selection if it is selected, otherwise adds
		* it as the new primary selection (Ctrl+click).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor が選択中なら選択から外し、そうでなければ新しい主な選択として
		* 加える（Ctrl+クリック）。
		*/
		void Toggle(Actor actor);

		/**
		* [EN]
		* Whether actor is selected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor が選択中か。
		*/
		Bool Contains(Actor actor)const;

		/**
		* [EN]
		* Deselects everything.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべての選択を解除する。
		*/
		void Clear();
	};
}
