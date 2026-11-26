#pragma once
#include <FoundationEngine/Log/LogSystem.h>

#define SC_LOG_WARNING(fmt, ...) \
	SeedCore::LogSystem::Push(SeedCore::LogLevel::Warning, std::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
