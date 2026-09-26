#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Small helper for querying filesystem locations relative to the
	* running process.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 実行中プロセスに関連するファイルシステムの場所を問い合わせるための
	* 小さなヘルパー。
	*/
	class SEEDCORE_API FileDirectory
	{
	public:
		/**
		* [EN]
		* Returns the directory containing the currently running
		* executable (no trailing separator). Returned as a path so callers
		* can join onto it with /; it also converts implicitly to
		* std::wstring where a wide string is needed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在実行中の実行ファイルを含むディレクトリを返す
		* （末尾の区切り文字なし）。呼び出し側が / で連結できるように
		* path で返す。ワイド文字列が必要な場所では std::wstring へ
		* 暗黙に変換される。
		*/
		static std::filesystem::path ExecutableDirectory();
	};
}