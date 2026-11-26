#pragma once
#include <FoundationEngine/World/Command/Command.h>

namespace SeedCore
{
	class SEEDCORE_API CompoundCommand : public Command
	{
	public:
		CompoundCommand() = default;

		CompoundCommand(const CompoundCommand&) = delete;
		CompoundCommand& operator=(const CompoundCommand&) = delete;
		CompoundCommand(CompoundCommand&&) = default;
		CompoundCommand& operator=(CompoundCommand&&) = default;

		void Add(ResourcePtr<Command> command);

		Bool Empty()const;

		void Redo()override;

		void Undo()override;

	private:
		DynamicArray<ResourcePtr<Command>> commands_;
	};
}
