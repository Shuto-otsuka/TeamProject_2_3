#pragma once
#include <FoundationEngine/Prelude.h>

/**
* [EN]
* Two-argument form of SC_STATIC_ASSERT: a thin wrapper over
* static_assert, named to match SC_ASSERT.
*
* ---------------------------------------------------------------------
*
* [JP]
* SC_STATIC_ASSERT の2引数版。SC_ASSERT と命名を揃えた static_assert の
* 薄いラッパー。
*/
#define SC_STATIC_ASSERT_2(cond, msg) static_assert(cond, msg)

/**
* [EN]
* Three-argument form of SC_STATIC_ASSERT: asserts that a C++ struct's
* size matches its HLSL mirror's, with a Japanese message generated from
* the type name and the HLSL file it mirrors, so every call site reports
* the mismatch in the same words.
*
* ---------------------------------------------------------------------
*
* [JP]
* SC_STATIC_ASSERT の3引数版。C++ 構造体のサイズが対応する HLSL 側と
* 一致することを表明する。型名とミラー先の HLSL ファイル名から日本語
* メッセージを自動生成するため、どの呼び出し箇所でも同じ言い回しで
* 不一致を報告する。
*/
#define SC_STATIC_ASSERT_3(type, size, hlslFile) static_assert(sizeof(type) == (size), #type " が " hlslFile " と一致していません")

/**
* [EN]
* Picks the macro name that sits in the NAME slot once the call's
* arguments are placed in front of the candidates: with two arguments
* SC_STATIC_ASSERT_2 lands there, with three SC_STATIC_ASSERT_3 does.
*
* ---------------------------------------------------------------------
*
* [JP]
* 呼び出しの引数を候補の前に並べたとき、NAME の位置に来るマクロ名を
* 取り出す。引数が2つなら SC_STATIC_ASSERT_2、3つなら SC_STATIC_ASSERT_3
* がそこに来る。
*/
#define SC_STATIC_ASSERT_GET_MACRO(_1, _2, _3, NAME, ...) NAME

/**
* [EN]
* Compile-time assertion, dispatched on the argument count:
* SC_STATIC_ASSERT(cond, msg) is a plain static_assert, and
* SC_STATIC_ASSERT(type, size, hlslFile) checks that type matches the
* byte size of its HLSL mirror. Arguments are counted by their commas,
* so a type whose name itself contains a comma (e.g. a template with two
* parameters) has to be given a single-word alias first.
*
* ---------------------------------------------------------------------
*
* [JP]
* 引数の数で振り分けるコンパイル時アサーション。
* SC_STATIC_ASSERT(cond, msg) は通常の static_assert、
* SC_STATIC_ASSERT(type, size, hlslFile) は type のサイズが HLSL 側の
* バイト数と一致するかを確かめる。引数はカンマで数えるので、名前に
* カンマを含む型（例: 引数2つのテンプレート）は、先に1語の別名を
* 付けてから渡すこと。
*/
#define SC_STATIC_ASSERT(...) SC_STATIC_ASSERT_GET_MACRO(__VA_ARGS__, SC_STATIC_ASSERT_3, SC_STATIC_ASSERT_2)(__VA_ARGS__)

/**
* [EN]
* Asserts that a C++ struct's size is a whole multiple of 16 bytes (one
* cbuffer row), with a generated Japanese message. For cbuffer-mirror
* structs whose HLSL side has no single fixed byte size to compare
* against directly.
*
* ---------------------------------------------------------------------
*
* [JP]
* C++ 構造体のサイズが 16 バイト(cbuffer の1行)の倍数であることを
* 表明する。日本語メッセージは自動生成する。HLSL 側に比較すべき単一の
* 固定バイト数が無い cbuffer ミラー構造体向け。
*/
#define SC_STATIC_ASSERT_ALIGNED16(type) static_assert(sizeof(type) % 16 == 0, #type " が 16 バイト行の倍数ではありません")

/**
* [EN]
* Runtime assertion, active in every build configuration. If cond is
* false, reports via HandleAssert (optionally with a formatted message)
* and terminates. Wrapped in a do { } while (false) so it behaves like
* a single statement at the call site (safe inside unbraced if/else).
*
* ---------------------------------------------------------------------
*
* [JP]
* 実行時アサーション。どのビルド構成でも有効。cond が偽の場合、HandleAssert（任意で整形済み
* メッセージ付き）を通じて報告し、終了する。呼び出し側で単一の文として
* 振る舞うよう do { } while (false) でラップしている（波括弧なしの
* if/else 内でも安全）。
*/
#define SC_ASSERT(cond, ...) \
		do { \
			if (!(cond)) { \
				SeedCore::HandleAssert(#cond, std::source_location::current(), ##__VA_ARGS__); \
			} \
		} while (false)

namespace SeedCore
{
	/**
	* [EN]
	* Reports a failed assertion with a formatted message to stderr, then
	* calls std::terminate. Called by SC_ASSERT when a message/format
	* arguments are supplied.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 整形済みメッセージ付きでアサーション失敗を stderr に報告し、
	* std::terminate を呼ぶ。メッセージ/フォーマット引数が渡された場合に
	* SC_ASSERT から呼ばれる。
	*/
	template<typename... Args>
	void HandleAssert(const Char* cond, const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args)
	{
		/// [EN] The condition text, location and message are written in one call so they stay together in the output.
		/// [JP] 条件の文字列・位置・メッセージを1回で書き出し、出力の中でまとまったままにする。
		std::cerr << std::format("[Assert Failed] {}\nFile: {}\nLine: {}\nMessage: {}\n", cond, loc.file_name(), loc.line(), std::format(fmt, std::forward<Args>(args)...));

		/// [EN] A broken invariant makes carrying on unsafe, so the process stops here.
		/// [JP] 前提が崩れたまま続けるのは危険なので、ここでプロセスを止める。
		std::terminate();
	}

	/**
	* [EN]
	* Reports a failed assertion without a message to stderr, then calls
	* std::terminate. Called by SC_ASSERT when no message/format
	* arguments are supplied.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メッセージなしでアサーション失敗を stderr に報告し、std::terminate
	* を呼ぶ。メッセージ/フォーマット引数が渡されなかった場合に
	* SC_ASSERT から呼ばれる。
	*/
	inline void HandleAssert(const Char* cond, const std::source_location& loc)
	{
		std::cerr << std::format("[Assert Failed] {}\nFile: {}\nLine: {}\n", cond, loc.file_name(), loc.line());

		/// [EN] A broken invariant makes carrying on unsafe, so the process stops here.
		/// [JP] 前提が崩れたまま続けるのは危険なので、ここでプロセスを止める。
		std::terminate();
	}
}