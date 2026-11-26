#pragma once

namespace SeedCore
{
	template <typename T>
	struct Handle
	{
		Uint64 index_ = std::numeric_limits<Uint64>::max();

		Uint64 generation_ = 0;

		SC_CONSTEX Handle null()noexcept
		{
			return {};
		}

		constexpr Bool exists()const noexcept
		{
			return index_ != std::numeric_limits<Uint64>::max();
		}

		constexpr Bool empty()const noexcept
		{
			return !exists();
		}

		friend Bool operator==(const Handle&, const Handle&) = default;
	};
}