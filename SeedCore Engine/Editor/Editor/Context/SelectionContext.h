#pragma once
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	struct SelectionContext
	{
		DynamicArray<Actor> actors_;

		Actor Primary()const;

		void Select(Actor actor);

		void Toggle(Actor actor);

		void Add(Actor actor);

		Bool Contains(Actor actor)const;

		void Clear();
	};
}
