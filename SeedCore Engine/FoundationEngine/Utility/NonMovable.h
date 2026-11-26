#pragma once

namespace SeedCore
{
	class SEEDCORE_API NonMovable
	{
	protected:
		constexpr NonMovable() = default;

		~NonMovable() = default;

	public:
		NonMovable(const NonMovable&) = default;
		NonMovable& operator=(const NonMovable&) = default;

		NonMovable(NonMovable&&) = delete;
		NonMovable& operator=(NonMovable&&) = delete;
	};
}