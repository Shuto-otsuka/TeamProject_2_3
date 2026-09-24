#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Mixin that deletes move construction/assignment while keeping copy
	* construction/assignment. Inherit from this to make a derived type
	* copy-only; the constructor and destructor are protected so the
	* mixin is never created or deleted on its own.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ムーブ構築/代入を削除し、コピー構築/代入は残す mixin。これを継承する
	* ことで、派生型をコピー専用にできる。コンストラクタとデストラクタは
	* protected なので、mixin 単体で作ったり消したりすることはできない。
	*/
	class NonMovable
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
		constexpr NonMovable() = default;

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
		~NonMovable() = default;

	public:
		/// [EN] Copying stays available.
		/// [JP] コピーは使えるまま残す。
		NonMovable(const NonMovable&) = default;
		NonMovable& operator=(const NonMovable&) = default;

		/// [EN] Moving is removed; a derived type's implicit move is then deleted too, so moving it copies instead.
		/// [JP] ムーブを削除する。派生型の暗黙のムーブも削除扱いになり、ムーブしようとするとコピーになる。
		NonMovable(NonMovable&&) = delete;
		NonMovable& operator=(NonMovable&&) = delete;
	};
}