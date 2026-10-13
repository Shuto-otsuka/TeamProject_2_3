#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	struct  EditorContext;

	class CompoundCommand;

	class ActorEditing
	{
	public:
		static void DeleteActor(EditorContext& context, Actor actor, CompoundCommand* group = nullptr);

		static void DeleteSelection(EditorContext& context);

		static void DuplicationSelection(EditorContext& context);
	};
}