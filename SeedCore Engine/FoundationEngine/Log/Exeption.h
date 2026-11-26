#pragma once

#define SC_THROW(fmt, ...) \
	throw SeedCore::Exception(std::format(fmt, ##__VA_ARGS__), std::source_location::current())

namespace SeedCore
{
	class Exception : public std::runtime_error
	{
	public:
		Exception(const std::string& message, const std::source_location& loc) : std::runtime_error(std::format("[{}:{}] {}", loc.file_name(), loc.line(), message))
		{
			/// No Code
		}
	};
}