#include <Editor/Editor/Context/SelectionContext.h>

namespace SeedCore
{
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
	Actor SelectionContext::Primary()const
	{
		return actors_.empty() ? Actor() : actors_.back();
	}

	/**
	* [EN]
	* Makes actor the only selection; an invalid actor clears it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor だけを選択した状態にする。無効な actor なら選択を空にする。
	*/
	void SelectionContext::Select(Actor actor)
	{
		actors_.clear();
		if (actor)
		{
			actors_.push_back(actor);
		}
	}

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
	void SelectionContext::Toggle(Actor actor)
	{
		if (!actor)
		{
			return;
		}

		auto it = std::ranges::find(actors_, actor);
		if (it != actors_.end())
		{
			actors_.erase(it);
			return;
		}

		actors_.push_back(actor);
	}

	/**
	* [EN]
	* Whether actor is selected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor が選択中か。
	*/
	Bool SelectionContext::Contains(Actor actor)const
	{
		return std::ranges::contains(actors_, actor);
	}

	/**
	* [EN]
	* Deselects everything.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべての選択を解除する。
	*/
	void SelectionContext::Clear()
	{
		actors_.clear();
	}
}
