#pragma once

namespace SeedCore
{
	template<typename T>
	struct Read
	{
		using Type = const T;
	};

	template<typename T>
	concept IsReadAccess = requires{typename T::Type;} && std::is_const_v<typename T::Type>;

	template<typename T>
	struct Write
	{
		using Type = T;
	};

	template<typename T>
	concept IsWriteAccess = requires{typename T::Type;} && !std::is_const_v<typename T::Type>;
}