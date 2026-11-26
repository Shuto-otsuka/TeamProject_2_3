#pragma once

namespace SeedCore
{
	template<typename T>
	concept StringLike = std::convertible_to<T, String>;
}