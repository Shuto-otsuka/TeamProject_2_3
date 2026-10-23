#include <Editor/Editor/Interaction/ActorEditing.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>
#include <FoundationEngine/World/Command/ActorCommand.h>
#include <FoundationEngine/World/Command/CompoundCommand.h>
#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>
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
			Bool ancestorSelected = std::ranges::any_of(toDuplicate, [&](Actor other)
				{
					for (Actor ancestor = actor.Parent(); ancestor; ancestor = ancestor.Parent())
					{
						if (ancestor == other)
						{
							return true;
						}
					}
					return false;
				});
			if (ancestorSelected)
			{
				continue;
			}

			PrefabBlueprint prefab;
			prefab.Capture(actor);

			const PrefabRegistry* prefabs = context.world_.world_->GetService<PrefabRegistry>();
			const PrefabRecord* prefabRecord = prefabs ? prefabs->Record(ActorAccessor::GetEntity(actor).GetID()) : nullptr;
			Actor duplicate = prefab.Instantiate(*context.world_.world_, *context.world_.resource_, actor.Parent(), prefabRecord ? prefabRecord->assetID_ : 0);
			if (!duplicate)
			{
				continue;
			}

			DynamicArray<BlueprintNode> nodes;
			BlueprintNode::Capture(duplicate, nodes);
			Actor duplicateParent = duplicate.Parent();
			ActorRegistry& actors = WorldAccessor::Actors(*context.world_.world_);
			Uint32 duplicateParentPersistentID = duplicateParent ? actors.Record(ActorAccessor::GetEntity(duplicateParent).GetID())->persistentID_ : 0;
			Uint32 actorPersistentID = actors.Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_;
			group->Add(MakePtr<ActorCreateCommand>(*context.world_.world_, *context.world_.resource_, nodes, duplicateParentPersistentID, actorPersistentID));

			duplicate.Move(actor);

			newSelection.push_back(duplicate);
		}

		if (!group->Empty())
		{
			context.scene_.history_.Push(std::move(group));
		}

		context.selection_.actors_ = newSelection;
	}
}