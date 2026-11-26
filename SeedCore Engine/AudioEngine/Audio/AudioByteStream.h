#pragma once

namespace SeedCore
{
	/**
	* [JP]
	* 音のデータをメモリから読めるようにする仕組み。
	*
	* @brief メモリから音を読めるようにする
	*
	* [EN]
	* Lets sound data be read straight from memory.
	*
	* @brief Lets sound be read from memory
	*/
	class SEEDCORE_API AudioByteStream final
	{
	public:
		/**
		* [JP]
		* メモリから読む仕組みを音のライブラリに登録する。
		*
		* @brief 仕組みを登録する
		*
		* [EN]
		* Registers the memory reader with the sound library.
		*
		* @brief Registers the reader
		*/
		static void Install();

		/**
		* [JP]
		* 登録した仕組みを外す。
		*
		* @brief 仕組みを外す
		*
		* [EN]
		* Removes the registered reader.
		*
		* @brief Removes the reader
		*/
		static void Uninstall();

	private:
		class Impl;
	};
}
