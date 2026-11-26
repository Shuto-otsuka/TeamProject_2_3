#pragma once

namespace SeedCore
{
	class SEEDCORE_API NonCopyable
	{
	protected:
		constexpr NonCopyable() = default;

		~NonCopyable() = default;

	public:
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;

		NonCopyable(NonCopyable&&) = default;
		NonCopyable& operator=(NonCopyable&&) = default;
	};
}