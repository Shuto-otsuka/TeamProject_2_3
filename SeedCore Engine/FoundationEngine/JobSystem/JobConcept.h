#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/// [EN] Satisfied by any type convertible to String (e.g. task/node names).
	/// [JP] String に変換可能な任意の型（タスク/ノード名など）を満たす。
	template<typename T>
	concept StringLike = std::convertible_to<T, String>;
}