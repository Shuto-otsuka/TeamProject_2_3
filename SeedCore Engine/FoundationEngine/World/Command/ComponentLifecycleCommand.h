#pragma once
#include <FoundationEngine/World/Command/Command.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>

namespace SeedCore
{
	class World;

	class SEEDCORE_API ComponentAddCommand : public Command
	{
	public:
		ComponentAddCommand(World& world, Uint32 actorPersistentId, ComponentID componentID);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;
		Uint32 actorPersistentId_;
		ComponentID componentID_;
	};

	class SEEDCORE_API ComponentRemoveCommand : public Command
	{
	public:
		ComponentRemoveCommand(World& world, Uint32 actorPersistentId, ComponentID componentID, const String& componentName, void* componentData);

		void Redo()override;

		void Undo()override;

	private:
		World& world_;
		Uint32 actorPersistentId_;
		ComponentID componentID_;
		BlueprintComponent captured_;
	};
}
