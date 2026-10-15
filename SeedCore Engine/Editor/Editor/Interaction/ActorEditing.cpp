#include <Editor/Editor/Interaction/ActorEditing.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Command/ActorCommand.h>
#include <FoundationEngine/World/Command/CompoundCommand.h>
#include <FoundationEngine/World/Actor/Blueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	void ActorEditing::DeleteActor(EditorContext& context, Actor actor, CompoundCommand* group)
	{
		SeedCore::erase_if(context.selection_.actors_, [actor](Actor selected) { return selected == actor; });

		ResourcePtr<Command> command = MakePtr<ActorDeleteCommand>(*context.world_.world_, *context.world_.resource_, actor);
		if (group)
		{
			group->Add(std::move(command));
		}
		else
		{
			context.scene_.history_.Push(std::move(command));
		}

		context.world_.world_->DestroyActor(actor);
	}

	void ActorEditing::DeleteSelection(EditorContext& context)
	{
		DynamicArray<Actor> toDelete = context.selection_.actors_;

		ResourcePtr<CompoundCommand> group = MakePtr<CompoundCommand>();
		for (Actor actor : toDelete)
		{
			DeleteActor(context, actor, group.get());
		}

		if (!group->Empty())
		{
			context.scene_.history_.Push(std::move(group));
		}
	}

	void ActorEditing::DuplicationSelection(EditorContext& context)
	{
		if (context.selection_.actors_.empty())
		{
			return;
		}

		DynamicArray<Actor> toDuplicate = context.selection_.actors_;
		DynamicArray<Actor> newSelection;

		ResourcePtr<CompoundCommand> group = MakePtr<CompoundCommand>();

		for (Actor actor : toDuplicate)
		{
			Bool ancestorSelected = std::ranges::any_of(toDuplicate, [&](Actor other) { return other != actor && actor.Descendant(other); });
			if (ancestorSelected)
			{
				continue;
			}

			PrefabBlueprint prefab;
			prefab.Capture(actor);

			Actor duplicate = prefab.Instantiate(*context.world_.world_, *context.world_.resource_, actor.Parent(), actor.PrefabID());
			if (!duplicate)
			{
				continue;
			}

			DynamicArray<BlueprintNode> nodes;
			CaptureActorNode(duplicate, -1, nodes);
			Actor duplicateParent = duplicate.Parent();
			group->Add(MakePtr<ActorCreateCommand>(*context.world_.world_, *context.world_.resource_, nodes, duplicateParent ? duplicateParent.PersistentID() : 0, actor.PersistentID()));

			Actor parent = actor.Parent();
			if (parent)
			{
				parent.MoveChild(duplicate, actor);
			}
			else
			{
				context.world_.world_->MoveActor(duplicate, actor);
			}

			newSelection.push_back(duplicate);
		}

		if (!group->Empty())
		{
			context.scene_.history_.Push(std::move(group));
		}

		context.selection_.actors_ = newSelection;
	}
}