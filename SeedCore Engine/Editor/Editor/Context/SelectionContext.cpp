#include <Editor/Editor/Context/SelectionContext.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>

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
		Clear();
		Add(actor);
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
			actor.RemoveComponent<Selected>();
			actors_.erase(it);
			return;
		}

		Add(actor);
	}

	/**
	* [EN]
	* Adds actor as the new primary selection unless it is already
	* selected; an invalid actor is ignored.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor が選択中でなければ、新しい主な選択として加える。無効な actor は
	* 無視する。
	*/
	void SelectionContext::Add(Actor actor)
	{
		if (!actor || Contains(actor))
		{
			return;
		}

		actor.AddComponent<Selected>();
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
		for (Actor actor : actors_)
		{
			if (actor)
			{
				actor.RemoveComponent<Selected>();
			}
		}
		actors_.clear();
	}
}
