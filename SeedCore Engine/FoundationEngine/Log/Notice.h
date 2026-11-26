#pragma once
#include <FoundationEngine/Log/LogSystem.h>

#define SC_LOG_NOTICE(fmt, ...) \
	SeedCore::LogSystem::Push(SeedCore::LogLevel::Notice, std::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
