#pragma once

#define SC_STATIC_ASSERT_2(cond, msg) static_assert(cond, msg)

#define SC_STATIC_ASSERT_3(type, size, hlslFile) static_assert(sizeof(type) == (size), #type " does not match " hlslFile)

#define SC_STATIC_ASSERT_GET_MACRO(_1, _2, _3, NAME, ...) NAME

#define SC_STATIC_ASSERT(...) SC_STATIC_ASSERT_GET_MACRO(__VA_ARGS__, SC_STATIC_ASSERT_3, SC_STATIC_ASSERT_2)(__VA_ARGS__)

#define SC_STATIC_ASSERT_ALIGNED16(type) static_assert(sizeof(type) % 16 == 0, #type " is not a multiple of 16 bytes")

#define SC_ASSERT(cond, ...) \
		do { \
			if (!(cond)) { \
				SeedCore::HandleAssert(#cond, std::source_location::current(), ##__VA_ARGS__); \
			} \
		} while (false)

namespace SeedCore
{
	template<typename... Args>
	void HandleAssert(const Char* cond, const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args)
	{
		std::cerr << std::format("[Assert Failed] {}\nFile: {}\nLine: {}\nMessage: {}\n", cond, loc.file_name(), loc.line(), std::format(fmt, std::forward<Args>(args)...));

		std::terminate();
	}

	inline void HandleAssert(const Char* cond, const std::source_location& loc)
	{
		std::cerr << std::format("[Assert Failed] {}\nFile: {}\nLine: {}\n", cond, loc.file_name(), loc.line());

		std::terminate();
	}
}