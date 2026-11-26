#pragma once

namespace SeedCore
{
	class SEEDCORE_API Command
	{
	public:
		virtual ~Command() = default;

		virtual void Redo() = 0;

		virtual void Undo() = 0;
	};
}
