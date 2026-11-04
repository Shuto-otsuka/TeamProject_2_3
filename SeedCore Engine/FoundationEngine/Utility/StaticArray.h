#pragma once
#include <FoundationEngine/Utility/Types.h>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace SeedCore
{
	/**
	* [EN]
	* Fixed-size, stack-allocated array of exactly N elements - the
	* project's std::array equivalent. N == 0 is handled by the
	* StaticArray<T, 0> specialization below. Like std::array it is an
	* aggregate (no user-declared constructors), so
	* StaticArray<Int, 3> a{ 1, 2, 3 }; works. Every member function is
	* constexpr, so a StaticArray can also be built, filled and read inside
	* constant evaluation (constexpr / consteval functions).
	*
	* The one deliberate difference from std::array: iterators are raw
	* pointers (iterator = T*). MSVC's std::array iterators are checked
	* wrapper objects under _ITERATOR_DEBUG_LEVEL (2 by default in
	* Debug), adding a bounds check to every dereference and increment;
	* the engine cannot globally disable that flag because the vendored
	* prebuilt libraries are compiled at the default level and MSVC
	* mangles symbol names by it. See DynamicArray for the full rationale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ちょうど N 要素の固定サイズ・スタック確保配列 - プロジェクトの
	* std::array 相当。N == 0 は下の StaticArray<T, 0> の特殊化が扱う。
	* std::array 同様、集成体（ユーザー宣言の
	* コンストラクタなし）なので StaticArray<Int, 3> a{ 1, 2, 3 }; が動く。
	* メンバー関数はすべて constexpr なので、定数評価の中（constexpr /
	* consteval の関数）でも作成・書き込み・読み出しができる。
	*
	* std::array との意図的な違いは1つ: イテレータは生ポインタ
	* （iterator = T*）。MSVC の std::array のイテレータは
	* _ITERATOR_DEBUG_LEVEL（Debug では既定で2）の下ではチェック付きの
	* ラッパーオブジェクトになり、逆参照・インクリメントごとに境界
	* チェックが入る。vendored のプリビルドライブラリが既定レベルで
	* コンパイル済みで MSVC がそのレベルでシンボル名を変えるため、
	* エンジン全体でこのフラグを無効にはできない。詳しい根拠は
	* DynamicArray 参照。
	*/
	template<typename T, Size N>
	struct StaticArray
	{
		/// [EN] The element type.
		/// [JP] 要素の型。
		using value_type = T;

		/// [EN] Type used for sizes and indices.
		/// [JP] サイズとインデックスに使う型。
		using size_type = Size;

		/// [EN] Signed type for the distance between two iterators.
		/// [JP] 2つのイテレータ間の距離を表す符号付き型。
		using difference_type = std::ptrdiff_t;

		/// [EN] Reference to a mutable element.
		/// [JP] 可変要素への参照。
		using reference = T&;

		/// [EN] Reference to a const element.
		/// [JP] const 要素への参照。
		using const_reference = const T&;

		/// [EN] Pointer to a mutable element.
		/// [JP] 可変要素へのポインタ。
		using pointer = T*;

		/// [EN] Pointer to a const element.
		/// [JP] const 要素へのポインタ。
		using const_pointer = const T*;

		/// [EN] Mutable iterator - a raw pointer, so it carries no debug-iterator overhead (see the class comment).
		/// [JP] 可変イテレータ - 生ポインタなので debug-iterator のオーバーヘッドが無い（クラスコメント参照）。
		using iterator = T*;

		/// [EN] Const iterator - a raw pointer to const.
		/// [JP] const イテレータ - const への生ポインタ。
		using const_iterator = const T*;

		/// [EN] Mutable reverse iterator.
		/// [JP] 可変逆イテレータ。
		using reverse_iterator = std::reverse_iterator<iterator>;

		/// [EN] Const reverse iterator.
		/// [JP] const 逆イテレータ。
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		/// [EN] The N contiguous elements. Public so the type stays an aggregate.
		/// [JP] 連続する N 個の要素。集成体であり続けるため public。
		T data_[N];

		/**
		* [EN]
		* Unchecked access to the element at index (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* index の要素への未チェックアクセス（const / 非 const）。
		*/
		constexpr reference operator[](Size index)
		{
			return data_[index];
		}

		constexpr const_reference operator[](Size index)const
		{
			return data_[index];
		}

		/**
		* [EN]
		* Bounds-checked access to the element at index; throws
		* std::out_of_range when index is past the end (const and non-const).
		* During constant evaluation the throw makes the expression
		* non-constant, so an out-of-range at() there is a compile error.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* index の要素への境界チェック付きアクセス。index が末尾を超えると
		* std::out_of_range を投げる（const / 非 const）。定数評価の中では
		* throw に届くと式が定数にならないので、範囲外の at() はコンパイル
		* エラーになる。
		*/
		constexpr reference at(Size index)
		{
			/// [EN] N is known at compile time, so the check is a single comparison with a constant.
			/// [JP] N はコンパイル時に決まっているので、確認は定数との比較1回で済む。
			if (index >= N)
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data_[index];
		}

		constexpr const_reference at(Size index)const
		{
			/// [EN] N is known at compile time, so the check is a single comparison with a constant.
			/// [JP] N はコンパイル時に決まっているので、確認は定数との比較1回で済む。
			if (index >= N)
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data_[index];
		}

		/**
		* [EN]
		* Returns the first element (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭要素を返す（const / 非 const）。
		*/
		constexpr reference front()
		{
			return data_[0];
		}

		constexpr const_reference front()const
		{
			return data_[0];
		}

		/**
		* [EN]
		* Returns the last element (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾要素を返す（const / 非 const）。
		*/
		constexpr reference back()
		{
			return data_[N - 1];
		}

		constexpr const_reference back()const
		{
			return data_[N - 1];
		}

		/**
		* [EN]
		* Returns a pointer to the element storage (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素ストレージへのポインタを返す（const / 非 const）。
		*/
		constexpr pointer data()noexcept
		{
			return data_;
		}

		constexpr const_pointer data()const noexcept
		{
			return data_;
		}

		/**
		* [EN]
		* Returns an iterator to the first element (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭要素へのイテレータを返す（const / 非 const）。
		*/
		constexpr iterator begin()noexcept
		{
			return data_;
		}

		constexpr const_iterator begin()const noexcept
		{
			return data_;
		}

		/**
		* [EN]
		* Returns a const iterator to the first element.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭要素への const イテレータを返す。
		*/
		constexpr const_iterator cbegin()const noexcept
		{
			return data_;
		}

		/**
		* [EN]
		* Returns an iterator one past the last element (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾要素の1つ後ろを指すイテレータを返す（const / 非 const）。
		*/
		constexpr iterator end()noexcept
		{
			return data_ + N;
		}

		constexpr const_iterator end()const noexcept
		{
			return data_ + N;
		}

		/**
		* [EN]
		* Returns a const iterator one past the last element.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾要素の1つ後ろを指す const イテレータを返す。
		*/
		constexpr const_iterator cend()const noexcept
		{
			return data_ + N;
		}

		/**
		* [EN]
		* Returns a reverse iterator to the last element (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾要素を指す逆イテレータを返す（const / 非 const）。
		*/
		constexpr reverse_iterator rbegin()noexcept
		{
			return reverse_iterator(end());
		}

		constexpr const_reverse_iterator rbegin()const noexcept
		{
			return const_reverse_iterator(end());
		}

		/**
		* [EN]
		* Returns a reverse iterator one before the first element (const and
		* non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭要素の1つ前を指す逆イテレータを返す（const / 非 const）。
		*/
		constexpr reverse_iterator rend()noexcept
		{
			return reverse_iterator(begin());
		}

		constexpr const_reverse_iterator rend()const noexcept
		{
			return const_reverse_iterator(begin());
		}

		/**
		* [EN]
		* Returns whether N is 0 - always false here, since N == 0 is handled
		* by the StaticArray<T, 0> specialization.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* N が0かを返す - N == 0 は StaticArray<T, 0> の特殊化が扱うため、
		* ここでは常に false。
		*/
		constexpr Bool empty()const noexcept
		{
			/// [EN] Kept for the standard container interface, so generic code can call it.
			/// [JP] 汎用のコードから呼べるよう、標準コンテナと同じ形で用意している。
			return N == 0;
		}

		/**
		* [EN]
		* Returns the element count, N.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素数 N を返す。
		*/
		constexpr Size size()const noexcept
		{
			return N;
		}

		/**
		* [EN]
		* Returns N (a fixed-size array's max size equals its size).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* N を返す（固定サイズ配列の最大サイズはサイズに等しい）。
		*/
		constexpr Size max_size()const noexcept
		{
			return N;
		}

		/**
		* [EN]
		* Assigns value to every element.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全要素に value を代入する。
		*/
		constexpr void fill(const T& value)
		{
			/// [EN] Plain assignment, since every element is already constructed.
			/// [JP] 全要素は構築済みなので、ただの代入で済む。
			for (Size index = 0; index < N; ++index)
			{
				data_[index] = value;
			}
		}

		/**
		* [EN]
		* Swaps contents element-wise with other.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other と要素ごとに内容を交換する。
		*/
		constexpr void swap(StaticArray& other)noexcept
		{
			/// [EN] The storage is inline, so contents cannot be exchanged by swapping pointers; every element is swapped.
			/// [JP] ストレージがインラインなので、ポインタの交換では済まず、全要素を交換する。
			for (Size index = 0; index < N; ++index)
			{
				std::swap(data_[index], other.data_[index]);
			}
		}

		/**
		* [EN]
		* Element-wise equality.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素ごとの等価。
		*/
		friend constexpr Bool operator==(const StaticArray& lhs, const StaticArray& rhs)
		{
			/// [EN] Written as !(a == b) so T only needs operator==; operator!= is generated from this by the compiler.
			/// [JP] !(a == b) の形にしているので、T には operator== だけあればよい。operator!= はこれからコンパイラが作る。
			for (Size index = 0; index < N; ++index)
			{
				if (!(lhs.data_[index] == rhs.data_[index]))
				{
					return false;
				}
			}
			return true;
		}
	};

	/**
	* [EN]
	* Zero-length StaticArray. A zero-length C array cannot be a member,
	* so this holds no storage at all: data(), begin() and end() are all
	* nullptr, so a loop over it runs zero times and it converts to an
	* empty std::span. It stays an aggregate, so StaticArray<T, 0> a{};
	* and an empty pack expansion { values... } both initialize it.
	* Element access is still declared so that generic code compiled for
	* every N also compiles for N == 0, but every index is out of range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 長さ 0 の StaticArray。長さ 0 の C 配列はメンバーにできないので、
	* ストレージを一切持たない。data()・begin()・end() はすべて nullptr
	* なので、これを回すループは 0 回で終わり、空の std::span にも変換
	* できる。集成体のままなので、StaticArray<T, 0> a{}; や、中身が空の
	* パック展開 { values... } で初期化できる。要素へのアクセスも宣言して
	* あるのは、すべての N でコンパイルされる汎用のコードが N == 0 でも
	* コンパイルできるようにするためで、どの index も範囲外になる。
	*/
	template<typename T>
	struct StaticArray<T, 0>
	{
		/// [EN] The element type.
		/// [JP] 要素の型。
		using value_type = T;

		/// [EN] Type used for sizes and indices.
		/// [JP] サイズとインデックスに使う型。
		using size_type = Size;

		/// [EN] Signed type for the distance between two iterators.
		/// [JP] 2つのイテレータ間の距離を表す符号付き型。
		using difference_type = std::ptrdiff_t;

		/// [EN] Reference to a mutable element.
		/// [JP] 可変要素への参照。
		using reference = T&;

		/// [EN] Reference to a const element.
		/// [JP] const 要素への参照。
		using const_reference = const T&;

		/// [EN] Pointer to a mutable element.
		/// [JP] 可変要素へのポインタ。
		using pointer = T*;

		/// [EN] Pointer to a const element.
		/// [JP] const 要素へのポインタ。
		using const_pointer = const T*;

		/// [EN] Mutable iterator - a raw pointer, the same type as in the general StaticArray.
		/// [JP] 可変イテレータ - 一般の StaticArray と同じ型の生ポインタ。
		using iterator = T*;

		/// [EN] Const iterator - a raw pointer to const.
		/// [JP] const イテレータ - const への生ポインタ。
		using const_iterator = const T*;

		/// [EN] Mutable reverse iterator.
		/// [JP] 可変逆イテレータ。
		using reverse_iterator = std::reverse_iterator<iterator>;

		/// [EN] Const reverse iterator.
		/// [JP] const 逆イテレータ。
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		/**
		* [EN]
		* Unchecked access to the element at index (const and non-const).
		* There is no element, so this must never be evaluated; it exists
		* only so generic code compiles.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* index の要素への未チェックアクセス（const / 非 const）。要素が
		* 無いので評価されてはならない。汎用のコードがコンパイルできる
		* ようにするためだけにある。
		*/
		constexpr reference operator[](Size index)
		{
			return data()[index];
		}

		constexpr const_reference operator[](Size index)const
		{
			return data()[index];
		}

		/**
		* [EN]
		* Bounds-checked access to the element at index; every index is past
		* the end, so this always throws std::out_of_range (const and
		* non-const). During constant evaluation that is a compile error.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* index の要素への境界チェック付きアクセス。どの index も末尾を
		* 超えるので、必ず std::out_of_range を投げる（const / 非 const）。
		* 定数評価の中ではコンパイルエラーになる。
		*/
		constexpr reference at(Size index)
		{
			/// [EN] Written as the same check as the general StaticArray, so the function reads the same for every N.
			/// [JP] 一般の StaticArray と同じ確認の形で書き、どの N でも同じ読み方になるようにしている。
			if (index >= size())
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data()[index];
		}

		constexpr const_reference at(Size index)const
		{
			/// [EN] Written as the same check as the general StaticArray, so the function reads the same for every N.
			/// [JP] 一般の StaticArray と同じ確認の形で書き、どの N でも同じ読み方になるようにしている。
			if (index >= size())
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data()[index];
		}

		/**
		* [EN]
		* Returns the first element (const and non-const). There is none, so
		* this must never be evaluated.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭要素を返す（const / 非 const）。要素が無いので評価されては
		* ならない。
		*/
		constexpr reference front()
		{
			return data()[0];
		}

		constexpr const_reference front()const
		{
			return data()[0];
		}

		/**
		* [EN]
		* Returns the last element (const and non-const). There is none, so
		* this must never be evaluated.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾要素を返す（const / 非 const）。要素が無いので評価されては
		* ならない。
		*/
		constexpr reference back()
		{
			return data()[0];
		}

		constexpr const_reference back()const
		{
			return data()[0];
		}

		/**
		* [EN]
		* Returns nullptr, since there is no element storage (const and
		* non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素ストレージが無いので nullptr を返す（const / 非 const）。
		*/
		constexpr pointer data()noexcept
		{
			return nullptr;
		}

		constexpr const_pointer data()const noexcept
		{
			return nullptr;
		}

		/**
		* [EN]
		* Returns nullptr as the first iterator, equal to end() (const and
		* non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭のイテレータとして、end() と等しい nullptr を返す
		* （const / 非 const）。
		*/
		constexpr iterator begin()noexcept
		{
			return nullptr;
		}

		constexpr const_iterator begin()const noexcept
		{
			return nullptr;
		}

		/**
		* [EN]
		* Returns nullptr as the first const iterator, equal to cend().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭の const イテレータとして、cend() と等しい nullptr を返す。
		*/
		constexpr const_iterator cbegin()const noexcept
		{
			return nullptr;
		}

		/**
		* [EN]
		* Returns nullptr as the end iterator, equal to begin() (const and
		* non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾のイテレータとして、begin() と等しい nullptr を返す
		* （const / 非 const）。
		*/
		constexpr iterator end()noexcept
		{
			return nullptr;
		}

		constexpr const_iterator end()const noexcept
		{
			return nullptr;
		}

		/**
		* [EN]
		* Returns nullptr as the end const iterator, equal to cbegin().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 末尾の const イテレータとして、cbegin() と等しい nullptr を返す。
		*/
		constexpr const_iterator cend()const noexcept
		{
			return nullptr;
		}

		/**
		* [EN]
		* Returns a reverse iterator equal to rend() (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* rend() と等しい逆イテレータを返す（const / 非 const）。
		*/
		constexpr reverse_iterator rbegin()noexcept
		{
			return reverse_iterator(end());
		}

		constexpr const_reverse_iterator rbegin()const noexcept
		{
			return const_reverse_iterator(end());
		}

		/**
		* [EN]
		* Returns a reverse iterator equal to rbegin() (const and non-const).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* rbegin() と等しい逆イテレータを返す（const / 非 const）。
		*/
		constexpr reverse_iterator rend()noexcept
		{
			return reverse_iterator(begin());
		}

		constexpr const_reverse_iterator rend()const noexcept
		{
			return const_reverse_iterator(begin());
		}

		/**
		* [EN]
		* Returns whether the array is empty - always true.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空かどうかを返す - 常に true。
		*/
		constexpr Bool empty()const noexcept
		{
			return true;
		}

		/**
		* [EN]
		* Returns the element count, 0.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素数 0 を返す。
		*/
		constexpr Size size()const noexcept
		{
			return 0;
		}

		/**
		* [EN]
		* Returns 0 (a fixed-size array's max size equals its size).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 0 を返す（固定サイズ配列の最大サイズはサイズに等しい）。
		*/
		constexpr Size max_size()const noexcept
		{
			return 0;
		}

		/**
		* [EN]
		* Assigns a value to every element - there is none, so nothing
		* happens.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全要素に値を代入する - 要素が無いので何もしない。
		*/
		constexpr void fill(const T&)
		{
			/// No Code
		}

		/**
		* [EN]
		* Swaps contents with another zero-length array - there is nothing
		* to swap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 別の長さ 0 の配列と内容を交換する - 交換するものが無い。
		*/
		constexpr void swap(StaticArray&)noexcept
		{
			/// No Code
		}

		/**
		* [EN]
		* Element-wise equality - two empty arrays are always equal.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素ごとの等価 - 空の配列同士は常に等しい。
		*/
		friend constexpr Bool operator==(const StaticArray&, const StaticArray&)
		{
			return true;
		}
	};

	/**
	* [EN]
	* Swaps two StaticArrays; found by ADL.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2つの StaticArray を交換する。ADL で見つかる。
	*/
	template<typename T, Size N>
	constexpr void swap(StaticArray<T, N>& lhs, StaticArray<T, N>& rhs)noexcept
	{
		/// [EN] Lets "using std::swap; swap(a, b);" pick the element-wise member swap.
		/// [JP] "using std::swap; swap(a, b);" の形で呼ばれたときに、要素ごとのメンバーの swap が選ばれるようにする。
		lhs.swap(rhs);
	}
}
