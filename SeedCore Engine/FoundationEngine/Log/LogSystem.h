#pragma once

namespace SeedCore
{
	enum class LogLevel : Uint8
	{
		Notice,

		Warning,

		Error
	};

	struct LogEntry
	{
		LogLevel level_;

		String message_;

		String file_;

		Int line_;
	};

	class SEEDCORE_API LogSystem
	{
	public:
		static void Push(LogLevel level, const std::string& message, const Char* file, Int line);

		static const DynamicArray<LogEntry>& GetLogs();

		static void Clear();

	private:
		static DynamicArray<LogEntry> committedLogs_;

		static DynamicArray<LogEntry> pendingLogs_;

		static std::mutex mutex_;
	};
}
