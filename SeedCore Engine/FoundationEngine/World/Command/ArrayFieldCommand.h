#pragma once
#include <FoundationEngine/World/Command/Command.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	class World;

	class SEEDCORE_API ArrayFieldCommand : public Command
	{
	protected:
		ArrayFieldCommand(World& world, Entity entity, ComponentID componentID, String fieldName, Size index);

	protected:
		World& world_;

		Entity entity_;

		ComponentID componentID_;

		String fieldName_;

		Size index_;
	};

	class SEEDCORE_API ArrayAppendCommand : public ArrayFieldCommand
	{
	public:
		ArrayAppendCommand(World& world, Entity entity, ComponentID componentID, String fieldName, Size index);

		void Redo()override;

		void Undo()override;
	};

	class SEEDCORE_API PayloadArrayCommand : public ArrayFieldCommand
	{
	public:
		PayloadArrayCommand(World& world, Entity entity, ComponentID componentID, String fieldName, Size index, Int value, Bool addOnRedo);

		void Redo()override;

		void Undo()override;

	private:
		Int value_;

		Bool addOnRedo_;
	};
}
