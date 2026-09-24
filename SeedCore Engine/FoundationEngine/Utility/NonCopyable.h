#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Mixin that deletes copy construction/assignment while keeping move
	* construction/assignment. Inherit from this to make a derived type
	* move-only; the constructor and destructor are protected so the
	* mixin is never created or deleted on its own.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コピー構築/代入を削除し、ムーブ構築/代入は残す mixin。これを継承する
	* ことで、派生型をムーブ専用にできる。コンストラクタとデストラクタは
	* protected なので、mixin 単体で作ったり消したりすることはできない。
	*/
	class NonCopyable
	{
	protected:
		/**
		* [EN]
		* Default constructor; constexpr so derived types can still be
		* constant-initialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ。派生型が定数初期化できるよう constexpr に
		* している。
		*/
		constexpr NonCopyable() = default;

		/**
		* [EN]
		* Non-virtual destructor: the mixin is never deleted through a
		* pointer to it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想ではないデストラクタ。mixin へのポインタ経由で削除されることは
		* 無い。
		*/
		~NonCopyable() = default;

	public:
		/// [EN] Copying is removed, which also removes it from every derived type.
		/// [JP] コピーを削除する。これにより、全ての派生型からもコピーが消える。
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;

		/// [EN] Moving stays available, and has to be declared explicitly once the copy operations are deleted.
		/// [JP] ムーブは使えるまま残す。コピーを削除した以上、明示的に宣言する必要がある。
		NonCopyable(NonCopyable&&) = default;
		NonCopyable& operator=(NonCopyable&&) = default;
	};
}