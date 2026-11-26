#pragma once

namespace sl
{
	enum class Result;
}

#define SC_SL_CHECK(sr, msg) SeedCore::SlFail(sr, msg, __FILE__, __LINE__)

namespace SeedCore
{
	SEEDCORE_API std::string SlResultToString(sl::Result sr);

	SEEDCORE_API void SlFail(sl::Result sr, const std::string& msg, const Char* file, Int line);
}
