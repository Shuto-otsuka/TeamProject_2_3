#pragma once
#include <FoundationEngine/Log/LogSystem.h>

#define SC_LOG_ERROR(fmt, ...) \
	SeedCore::LogSystem::Push(SeedCore::LogLevel::Error, std::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
