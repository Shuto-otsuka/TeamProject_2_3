#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Mixin that deletes both copy and move construction/assignment.
	* Inherit from this to make a derived type neither copyable nor
	* movable, so each object keeps its identity and address for its
	* whole life; the constructor and destructor are protected so the
	* mixin is never created or deleted on its own.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コピーとムーブの両方の構築/代入を削除する mixin。これを継承すること
	* で、派生型をコピーもムーブもできなくし、各オブジェクトが生涯を通じて
	* 同じアイデンティティとアドレスを保つようにする。コンストラクタと
	* デストラクタは protected なので、mixin 単体で作ったり消したりする
	* ことはできない。
	*/
	class NonTransferable
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
		constexpr NonTransferable() = default;

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
		~NonTransferable() = default;

	public:
		/// [EN] Copying is removed.
		/// [JP] コピーを削除する。
		NonTransferable(const NonTransferable&) = delete;
		NonTransferable& operator=(const NonTransferable&) = delete;

		/// [EN] Moving is removed too, so pointers to the object stay valid for its whole life.
		/// [JP] ムーブも削除するので、オブジェクトへのポインタは生涯を通じて有効なまま。
		NonTransferable(NonTransferable&&) = delete;
		NonTransferable& operator=(NonTransferable&&) = delete;
	};
}