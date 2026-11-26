#pragma once

namespace SeedCore
{
	template <typename T, typename>
	struct GetIndex;

	template <Size I, typename... Ts>
	struct GetIndexImplementation {};

	template <Size I, typename T, typename... Ts>
	struct GetIndexImplementation<I, T, T, Ts...> : std::integral_constant<Size, I> {};

	template <Size I, typename T, typename U, typename... Ts>
	struct GetIndexImplementation<I, T, U, Ts...> : GetIndexImplementation<I + 1, T, Ts...> {};

	template <typename T, typename... Ts>
	struct GetIndex<T, std::variant<Ts...>> : GetIndexImplementation<0, T, Ts...> {};

	template <typename T, typename... Ts>
	constexpr auto GetIndexValue = GetIndex<T, Ts...>::value;
}
