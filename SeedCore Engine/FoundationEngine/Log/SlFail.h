#pragma once

namespace sl
{
	enum class Result;
}

/**
* [EN]
* Checks an sl::Result from a Streamline (DLSS) call by handing it to
* SlFail together with the file and line of the call site; SlFail
* itself lets eOk through.
*
* ---------------------------------------------------------------------
*
* [JP]
* Streamline（DLSS）呼び出しの sl::Result を、呼び出し元のファイル/行と
* 一緒に SlFail へ渡して検査する。eOk を素通りさせるのは SlFail 側。
*/
#define SC_SL_CHECK(sr, msg) SeedCore::SlFail(sr, msg, __FILE__, __LINE__)

namespace SeedCore
{
	/**
	* [EN]
	* Converts an sl::Result to its enumerator name for display in error messages.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* sl::Result を、エラーメッセージ表示用にその列挙子名の文字列へ変換する。
	*/
	SEEDCORE_API std::string SlResultToString(sl::Result sr);

	/**
	* [EN]
	* Handles a failed sl::Result: does nothing if sr is eOk,
	* otherwise shows a blocking error message box with the failing
	* result/message and breaks into the debugger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 失敗した sl::Result を処理する。sr が eOk であれば何もしない。
	* それ以外は失敗した結果/メッセージを含むブロッキングのエラー
	* メッセージボックスを表示し、デバッガへブレークする。
	*/
	SEEDCORE_API void SlFail(sl::Result sr, const std::string& msg, const Char* file, Int line);
}
