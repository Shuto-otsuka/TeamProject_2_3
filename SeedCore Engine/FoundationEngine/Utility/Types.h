#pragma once
#include <cstddef>
#include <cstdint>

namespace SeedCore
{
	/// [EN] Signed integer aliases. Int is the default; the fixed-width ones pin an exact bit width (e.g. file formats).
	/// [JP] 符号付き整数のエイリアス。Int が既定で、固定幅版はビット幅を決めたい場合（ファイル形式など）に使う。
	using Int = int;
	using Int8 = std::int8_t;
	using Int16 = std::int16_t;
	using Int32 = std::int32_t;
	using Int64 = std::int64_t;

	/// [EN] Unsigned integer aliases, mirroring the Int group.
	/// [JP] Int 群に対応する符号なし整数のエイリアス。
	using Uint = unsigned int;
	using Uint8 = std::uint8_t;
	using Uint16 = std::uint16_t;
	using Uint32 = std::uint32_t;
	using Uint64 = std::uint64_t;

	/// [EN] Floating-point aliases.
	/// [JP] 浮動小数点数のエイリアス。
	using Float = float;
	using Double = double;

	/// [EN] The same type as Uint, named after the bare keyword for places that mirror a third-party signature.
	/// [JP] Uint と同じ型。サードパーティのシグネチャに合わせる箇所向けに、生のキーワードに合わせた名前にしている。
	using Unsigned = unsigned;

	/// [EN] long/long long aliases, for interop with APIs that use them explicitly.
	/// [JP] long/long long のエイリアス。それらを明示的に使うAPIとの連携用。
	using Long = long;
	using Longlong = long long;
	using Ulong = unsigned long;
	using Ulonglong = unsigned long long;

	/// [EN] short aliases, for interop with APIs that use them explicitly.
	/// [JP] short のエイリアス。それを明示的に使うAPIとの連携用。
	using Short = short;
	using Ushort = unsigned short;

	/// [EN] Boolean alias.
	/// [JP] 真偽値のエイリアス。
	using Bool = bool;

	/// [EN] Character aliases, one per string encoding.
	/// [JP] 文字のエイリアス。文字列エンコーディングごとに1つ。
	using Char = char;
	using Char8 = char8_t;
	using Char16 = char16_t;
	using Char32 = char32_t;
	using Wchar = wchar_t;

	/// [EN] Unsigned 8-bit character, mainly for <cctype> functions (e.g. std::tolower) whose argument must not be negative.
	/// [JP] 符号なし8ビット文字。主に std::tolower など、引数が負であってはならない <cctype> 系関数に使う。
	using Uchar = unsigned char;

	/// [EN] Alias for std::size_t, used for sizes/counts/indices.
	/// [JP] std::size_t のエイリアス。サイズ・個数・インデックスに使う。
	using Size = std::size_t;

	/// [EN] Raw memory bytes; the same char as Char underneath, but Byte is for memcpy/binary I/O and Char for text.
	/// [JP] 生メモリのバイト。中身は Char と同じ char だが、Byte は memcpy やバイナリ I/O に、Char は文字列に使う。
	using Byte = char;
}