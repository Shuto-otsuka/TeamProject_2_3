#include <FoundationEngine/World/Command/CompoundCommand.h>

namespace SeedCore
{
	void CompoundCommand::Add(ResourcePtr<Command> command)
	{
		commands_.push_back(std::move(command));
	}

	Bool CompoundCommand::Empty()const
	{
		return commands_.empty();
	}

	void CompoundCommand::Redo()
	{
		for (Size index = 0; index < commands_.size(); ++index)
		{
			commands_[index]->Redo();
		}
	}

	void CompoundCommand::Undo()
	{
		for (Size index = commands_.size(); index > 0; --index)
		{
			commands_[index - 1]->Undo();
		}
	}
}
