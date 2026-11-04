#include <FoundationEngine/Application/Bootstrap.h>

namespace SeedCore
{
	/**
	* [EN]
	* Switches the C runtime's character encoding to UTF-8, so narrow
	* strings used as paths are read as UTF-8 and paths containing
	* non-ASCII characters (e.g. Japanese folder names) resolve correctly.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* C ランタイムの文字コードを UTF-8 に切り替える。これにより、パスと
	* して使う char 文字列が UTF-8 として解釈され、日本語のフォルダ名など
	* 非 ASCII 文字を含むパスも正しく扱える。
	*/
	Bootstrap::Bootstrap()
	{
		/// [EN] String holds UTF-8, while std::filesystem and the C runtime convert narrow strings with the locale's code page; making that code page UTF-8 lets both agree. Only LC_CTYPE is changed, leaving number formatting and other categories as they are. The CRT is shared as a DLL, so this one call applies to every engine module.
		/// [JP] String は UTF-8 を持つ一方、std::filesystem と C ランタイムは char 文字列をロケールのコードページで変換する。そのコードページを UTF-8 にすることで両者が一致する。変えるのは LC_CTYPE だけで、数値の書式など他のカテゴリはそのまま。CRT は DLL として共有されるので、この1回の呼び出しがエンジンの全モジュールに効く。
		setlocale(LC_CTYPE, ".UTF8");
	}
}