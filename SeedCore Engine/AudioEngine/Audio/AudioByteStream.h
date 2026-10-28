#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* File I/O that CRI File System uses instead of its own. It serves two
	* kinds of path: plain paths are read straight from disk, and paths
	* starting with "seedcore_audio://" point at a .audio cache whose
	* stream blob (the AWB) is AES-256-CBC encrypted, decrypted only over
	* the range CRI asks for. The CRI callbacks live in Impl, so this header
	* pulls in no CRI types.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CRI File System が自前の入出力の代わりに使うファイル I/O。2種類の
	* パスを扱う: 普通のパスはディスクからそのまま読み、"seedcore_audio://"
	* で始まるパスは、ストリーム用ブロブ(AWB)が AES-256-CBC で暗号化された
	* .audio キャッシュを指し、CRI が求めた範囲だけを復号する。CRI の
	* コールバックは Impl にあるので、このヘッダは CRI の型を取り込まない。
	*/
	class SEEDCORE_API AudioByteStream final
	{
	public:
		/**
		* [EN]
		* Registers this file I/O with CRI File System, so every file CRI
		* opens from then on goes through it. Called before CRI is
		* initialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このファイル I/O を CRI File System に登録し、以後 CRI が開く全ての
		* ファイルがここを通るようにする。CRI の初期化より前に呼ぶ。
		*/
		static void Install();

		/**
		* [EN]
		* Removes the registration, so CRI no longer calls into this class.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 登録を外し、CRI がこのクラスを呼ばないようにする。
		*/
		static void Uninstall();

	private:
		class Impl;
	};
}
