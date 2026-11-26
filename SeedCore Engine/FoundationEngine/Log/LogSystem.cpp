#include <FoundationEngine/Log/LogSystem.h>

namespace SeedCore
{
	DynamicArray<LogEntry> LogSystem::committedLogs_;

	DynamicArray<LogEntry> LogSystem::pendingLogs_;

	std::mutex LogSystem::mutex_;

	void LogSystem::Push(LogLevel level, const std::string& message, const Char* file, Int line)
	{
		LogEntry entry = { level, String(message), String(std::filesystem::path(file).filename().string()), line };

		std::lock_guard<std::mutex> lock(mutex_);
		pendingLogs_.push_back(std::move(entry));
	}

	const DynamicArray<LogEntry>& LogSystem::GetLogs()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (LogEntry& entry : pendingLogs_)
		{
			committedLogs_.push_back(std::move(entry));
		}
		pendingLogs_.clear();
		return committedLogs_;
	}

	void LogSystem::Clear()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		pendingLogs_.clear();
		committedLogs_.clear();
	}
}
