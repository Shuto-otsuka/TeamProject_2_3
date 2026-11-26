#pragma once
#include <FoundationEngine/World/Command/Command.h>

namespace SeedCore
{
	class SEEDCORE_API History
	{
	public:
		History() = default;

		History(const History&) = delete;
		History& operator=(const History&) = delete;
		History(History&&) = default;
		History& operator=(History&&) = default;

		void Push(ResourcePtr<Command> command);

		void Undo();

		void Redo();

		void Clear();

	private:
		SC_CONSTEX Size maxDepth_ = 100;

		DynamicArray<ResourcePtr<Command>> undoStack_;

		DynamicArray<ResourcePtr<Command>> redoStack_;
	};
}
