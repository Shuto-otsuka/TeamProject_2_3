#include <Editor/Editor/Context/SelectionContext.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>

namespace SeedCore
{
	Actor SelectionContext::Primary()const
	{
		return actors_.empty() ? Actor() : actors_.back();
	}

	void SelectionContext::Select(Actor actor)
	{
		Clear();
		Add(actor);
	}

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

	void SelectionContext::Add(Actor actor)
	{
		if (!actor || Contains(actor))
		{
			return;
		}

		actor.AddComponent<Selected>();
		actors_.push_back(actor);
	}

	Bool SelectionContext::Contains(Actor actor)const
	{
		return std::ranges::contains(actors_, actor);
	}

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
