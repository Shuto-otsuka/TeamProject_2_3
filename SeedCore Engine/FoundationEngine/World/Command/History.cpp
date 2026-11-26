#include <FoundationEngine/World/Command/History.h>

namespace SeedCore
{
	void History::Push(ResourcePtr<Command> command)
	{
		redoStack_.clear();

		undoStack_.push_back(std::move(command));

		if (undoStack_.size() > maxDepth_)
		{
			undoStack_.erase(undoStack_.begin());
		}
	}

	void History::Undo()
	{
		if (undoStack_.empty())
		{
			return;
		}

		ResourcePtr<Command> command = std::move(undoStack_.back());
		undoStack_.pop_back();

		command->Undo();

		redoStack_.push_back(std::move(command));
	}

	void History::Redo()
	{
		if (redoStack_.empty())
		{
			return;
		}

		ResourcePtr<Command> command = std::move(redoStack_.back());
		redoStack_.pop_back();

		command->Redo();

		undoStack_.push_back(std::move(command));
	}

	void History::Clear()
	{
		undoStack_.clear();
		redoStack_.clear();
	}
}
