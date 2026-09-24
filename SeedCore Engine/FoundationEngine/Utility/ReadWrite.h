#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Tag type marking a query/access parameter as read-only. Type
	* resolves to const T, used by query systems (e.g. ECS Query) to
	* determine component access mode at compile time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クエリ/アクセスパラメータを読み取り専用としてマークするタグ型。
	* Type は const T に解決され、クエリシステム（ECS の Query など）が
	* コンポーネントのアクセスモードをコンパイル時に判定するために使う。
	*/
	template<typename T>
	struct Read
	{
		/// [EN] The type a query hands out for this parameter: a const view of the component.
		/// [JP] クエリがこの引数に渡す型。コンポーネントの const な見え方。
		using Type = const T;
	};

	/// [EN] Satisfied by Read<T>-shaped tag types (i.e. Type is const-qualified); any tag with a const Type counts, not only Read.
	/// [JP] Read<T> 形のタグ型（すなわち Type が const 修飾されている）を満たす。Read に限らず、const な Type を持つタグなら当てはまる。
	template<typename T>
	concept IsReadAccess = requires{typename T::Type;} && std::is_const_v<typename T::Type>;

	/**
	* [EN]
	* Tag type marking a query/access parameter as read-write. Type
	* resolves to T (unqualified), used by query systems (e.g. ECS
	* Query) to determine component access mode at compile time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クエリ/アクセスパラメータを読み書き可能としてマークするタグ型。
	* Type は T（非修飾）に解決され、クエリシステム（ECS の Query など）が
	* コンポーネントのアクセスモードをコンパイル時に判定するために使う。
	*/
	template<typename T>
	struct Write
	{
		/// [EN] The type a query hands out for this parameter: the component itself, modifiable.
		/// [JP] クエリがこの引数に渡す型。変更できるコンポーネントそのもの。
		using Type = T;
	};

	/// [EN] Satisfied by Write<T>-shaped tag types (i.e. Type is not const-qualified).
	/// [JP] Write<T> 形のタグ型（すなわち Type が const 修飾されていない）を満たす。
	template<typename T>
	concept IsWriteAccess = requires{typename T::Type;} && !std::is_const_v<typename T::Type>;
}