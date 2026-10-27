#include <FoundationEngine/Log/LogSystem.h>

namespace SeedCore
{
	/// [EN] Definition of the shared log buffer declared in LogSystem.h.
	/// [JP] LogSystem.h で宣言した、共有のログバッファの定義。
	DynamicArray<LogEntry> LogSystem::committedLogs_;

	DynamicArray<LogEntry> LogSystem::pendingLogs_;

	std::mutex LogSystem::mutex_;

	/**
	* [EN]
	* Appends a new LogEntry built from level/message and the
	* calling file/line (file is reduced to its basename).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* level/message と呼び出し元のファイル/行から LogEntry を
	* 構築して追加する（file はベース名に縮約される）。
	*/
	void LogSystem::Push(LogLevel level, const std::string& message, const Char* file, Int line)
	{
		/// [EN] __FILE__ is a full build path; only the file name is kept, which is what the Console shows.
		/// [JP] __FILE__ はビルド時の完全なパスなので、Console に表示するファイル名だけを残す。
		LogEntry entry = { level, String(message), String(std::filesystem::path(file).filename().string()), line };

		std::lock_guard<std::mutex> lock(mutex_);
		pendingLogs_.push_back(std::move(entry));
	}

	/**
	* [EN]
	* Returns all entries logged so far, in push order.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* これまでに記録された全エントリを、追加順で返す。
	*/
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

	/**
	* [EN]
	* Removes all logged entries.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 記録済みの全エントリを削除する。
	*/
	void LogSystem::Clear()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		pendingLogs_.clear();
		committedLogs_.clear();
	}
}
