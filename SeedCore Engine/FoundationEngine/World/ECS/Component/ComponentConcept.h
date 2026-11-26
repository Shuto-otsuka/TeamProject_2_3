#pragma once

namespace SeedCore
{
	template<typename T>
	concept IsComponent = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
}
