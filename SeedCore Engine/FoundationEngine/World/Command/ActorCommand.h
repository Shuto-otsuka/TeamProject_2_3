#pragma once
#include <FoundationEngine/World/Command/Command.h>
#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class Actor;

	class SEEDCORE_API ActorCreateCommand : public Command
	{
	public:
		ActorCreateCommand(World& world, ResourceCache& cache, DynamicArray<BlueprintNode> nodes, Uint32 parentPersistentId = 0, Uint32 prevSiblingPersistentId = 0);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;

		ResourceCache& cache_;

		DynamicArray<BlueprintNode> nodes_;

		Uint32 parentPersistentId_;

		Uint32 prevSiblingPersistentId_;
	};

	class SEEDCORE_API ActorDeleteCommand : public Command
	{
	public:
		ActorDeleteCommand(World& world, ResourceCache& cache, Actor actor);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;

		ResourceCache& cache_;

		BlueprintNode node_;

		Uint32 persistentId_ = 0;

		Uint32 parentPersistentId_ = 0;

		DynamicArray<Uint32> childPersistentIds_;
	};

	class SEEDCORE_API ActorReparentCommand : public Command
	{
	public:
		ActorReparentCommand(World& world, Uint32 actorPersistentId, Uint32 oldParentPersistentId, Uint32 oldPrevSiblingPersistentId, Uint32 newParentPersistentId);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;

		Uint32 actorPersistentId_;

		Uint32 oldParentPersistentId_;

		Uint32 oldPrevSiblingPersistentId_;

		Uint32 newParentPersistentId_;
	};

	class SEEDCORE_API ActorTagCommand : public Command
	{
	public:
		ActorTagCommand(World& world, Uint32 actorPersistentId, const String& tag, Bool addOnRedo);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;

		Uint32 actorPersistentId_;

		String tag_;

		Bool addOnRedo_;
	};

	class SEEDCORE_API ActorLayerCommand : public Command
	{
	public:
		ActorLayerCommand(World& world, Uint32 actorPersistentId, const String& oldLayerName, const String& newLayerName);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;

		Uint32 actorPersistentId_;

		String oldLayerName_;

		String newLayerName_;
	};

	class SEEDCORE_API ActorActiveCommand : public Command
	{
	public:
		ActorActiveCommand(World& world, Actor actor, Bool newActive);

		void Redo()override;

		void Undo()override;

	private:
		struct Entry
		{
			Uint32 persistentId_ = 0;

			Bool oldActive_ = true;
		};

		World& world_;

		Uint32 rootPersistentId_;

		Bool newActive_;

		DynamicArray<Entry> entries_;
	};
}
