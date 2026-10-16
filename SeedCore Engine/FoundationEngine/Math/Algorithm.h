#pragma once
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <numbers>
#include <type_traits>

namespace SeedCore
{
	/**
	* [EN]
	* Compile-time collection of pi and its common derived constants for
	* floating-point type T (multiples/fractions, reciprocal, and
	* degree/radian conversion factors).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 浮動小数点型 T に対する、円周率とその代表的な派生定数（倍数/分数、
	* 逆数、度/ラジアン変換係数）をまとめたコンパイル時定数集。
	*/
	template<typename T>
		requires std::floating_point<T>
	struct Pi
	{
		/// [EN] The value of pi.
		/// [JP] 円周率の値。
		static constexpr T Value = std::numbers::pi_v<T>;

		/// [EN] Two times pi (a full turn in radians).
		/// [JP] 円周率の2倍（ラジアンでの1回転）。
		static constexpr T Two = Value * T(2);

		/// [EN] Four times pi.
		/// [JP] 円周率の4倍。
		static constexpr T Four = Value * T(4);

		/// [EN] Half of pi (a quarter turn in radians).
		/// [JP] 円周率の半分（ラジアンでの4分の1回転）。
		static constexpr T Half = Value * T(0.5);

		/// [EN] A quarter of pi.
		/// [JP] 円周率の4分の1。
		static constexpr T Quarter = Value * T(0.25);

		/// [EN] The reciprocal of pi.
		/// [JP] 円周率の逆数。
		static constexpr T Inverse = T(1) / Value;

		/// [EN] Multiplier converting degrees to radians.
		/// [JP] 度をラジアンへ変換する際に乗じる係数。
		static constexpr T ToRadian = Value / T(180);

		/// [EN] Multiplier converting radians to degrees.
		/// [JP] ラジアンを度へ変換する際に乗じる係数。
		static constexpr T ToDegree = T(180) / Value;
	};
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the greater of a and b.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* a と b のうち大きい方を返す。
	*/
	template<typename T>
		requires std::totally_ordered<T>
	[[nodiscard]] constexpr const T& Max(const T& a, const T& b)noexcept
	{
		return (a < b) ? b : a;
	}

	/**
	* [EN]
	* Returns the greatest of first and args (variadic fold via pairwise Max).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* first と args のうち最大のものを返す（Max によるペアワイズの
	* 可変長畳み込み）。
	*/
	template<typename T, typename... Args>
		requires std::totally_ordered<T>
	[[nodiscard]] constexpr std::common_type_t<T, Args...> Max(T first, Args... args)noexcept
	{
		/// [EN] Fold every argument into one common type and call the two-argument overload with it explicitly;
		///      with mixed types its deduction would fail and the call would recurse into this overload forever.
		/// [JP] 全引数を共通の型にそろえ、2引数版を明示的な型で呼ぶ。
		///      型が混ざったままだと2引数版の推論が失敗し、この可変長版へ無限に再帰する。
		using ReturnType = std::common_type_t<T, Args...>;

		if constexpr (sizeof...(args) == 0)
		{
			return static_cast<ReturnType>(first);
		}
		else
		{
			return Max<ReturnType>(static_cast<ReturnType>(first), static_cast<ReturnType>(Max(args...)));
		}
	}

	/**
	* [EN]
	* Returns the lesser of a and b.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* a と b のうち小さい方を返す。
	*/
	template<typename T>
		requires std::totally_ordered<T>
	[[nodiscard]] constexpr const T& Min(const T& a, const T& b)noexcept
	{
		return (b < a) ? b : a;
	}

	/**
	* [EN]
	* Returns the least of first and args (variadic fold via pairwise Min).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* first と args のうち最小のものを返す（Min によるペアワイズの
	* 可変長畳み込み）。
	*/
	template<typename T, typename... Args>
		requires std::totally_ordered<T>
	[[nodiscard]] constexpr std::common_type_t<T, Args...> Min(T first, Args... args) noexcept
	{
		/// [EN] Fold every argument into one common type and call the two-argument overload with it explicitly;
		///      with mixed types its deduction would fail and the call would recurse into this overload forever.
		/// [JP] 全引数を共通の型にそろえ、2引数版を明示的な型で呼ぶ。
		///      型が混ざったままだと2引数版の推論が失敗し、この可変長版へ無限に再帰する。
		using ReturnType = std::common_type_t<T, Args...>;

		if constexpr (sizeof...(args) == 0)
		{
			return static_cast<ReturnType>(first);
		}
		else
		{
			return Min<ReturnType>(static_cast<ReturnType>(first), static_cast<ReturnType>(Min(args...)));
		}
	}

}

namespace SeedCore
{
	/**
	* [EN]
	* Clamps value into the inclusive range [min, max].
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を [min, max] の閉区間へクランプする。
	*/
	template<typename T, typename U>
		requires std::totally_ordered<T>
	[[nodiscard]] constexpr T Clamp(const U& value, const T& min, const T& max)noexcept
	{
		using ReturnType = std::common_type_t<T, U>;
		return Max(static_cast<ReturnType>(min), Min(static_cast<ReturnType>(value), static_cast<ReturnType>(max)));
	}

	/**
	* [EN]
	* Wraps value into the range [min, max) using floating-point modulo,
	* looping around at the boundaries instead of clamping. Returns min
	* if the range is empty/inverted.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 浮動小数点の剰余を用いて value を [min, max) の範囲へラップする。
	* 境界でクランプせず巻き戻る。範囲が空/逆転している場合は min を返す。
	*/
	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::floating_point<T>
	[[nodiscard]] constexpr T Wrap(U value, T min, T max)noexcept
	{
		const T range = max - min;
		if (range <= 0) [[unlikely]]
		{
			return min;
		}

		T result = std::fmod(value - min, range);
		if (result < 0)
		{
			result += range;
		}
		return min + result;
	}

	/**
	* [EN]
	* Integer overload of Wrap: wraps value into the range [min, max)
	* using integer modulo. Returns min if the range is empty/inverted.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Wrap の整数版: 整数剰余を用いて value を [min, max) の範囲へ
	* ラップする。範囲が空/逆転している場合は min を返す。
	*/
	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::integral<T>
	[[nodiscard]] constexpr T Wrap(U value, T min, T max)noexcept
	{
		const T range = max - min;
		if (range <= 0) [[unlikely]]
		{
			return min;
		}

		T result = (value - min) % range;
		if (result < 0)
		{
			result += range;
		}
		return min + result;
	}

	/**
	* [EN]
	* Bounces value back and forth within [min, max] (ping-pong wrapping)
	* using floating-point modulo. Returns min if the range is empty/inverted.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 浮動小数点の剰余を用いて value を [min, max] 内で往復させる
	* （ピンポン方式のラップ）。範囲が空/逆転している場合は min を返す。
	*/
	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::floating_point<T>
	[[nodiscard]] constexpr T Pingpong(U value, T min, T max)noexcept
	{
		const T range = max - min;
		if (range <= 0) [[unlikely]]
		{
			return min;
		}

		const T doubleRange = range * 2;
		T result = std::fmod(value - min, doubleRange);
		if (result < 0)
		{
			result += doubleRange;
		}
		return min + (result > range ? (doubleRange - result) : result);
	}

	/**
	* [EN]
	* Integer overload of Pingpong: bounces value back and forth within
	* [min, max] using integer modulo. Returns min if the range is empty/inverted.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Pingpong の整数版: 整数剰余を用いて value を [min, max] 内で
	* 往復させる。範囲が空/逆転している場合は min を返す。
	*/
	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::integral<T>
	[[nodiscard]] constexpr T Pingpong(U value, T min, T max)noexcept
	{
		const T range = max - min;
		if (range <= 0) [[unlikely]]
		{
			return min;
		}

		const T doubleRange = range * 2;
		T result = (value - min) % doubleRange;
		if (result < 0)
		{
			result += doubleRange;
		}
		return min + (result > range ? (doubleRange - result) : result);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Linearly interpolates between min and max by t, clamping t to [0, 1] first.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* t を [0, 1] へクランプした上で、min と max の間を線形補間する。
	*/
	template<typename T, typename U>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Lerp(T min, T max, U t)noexcept
	{
		if (t <= 0) [[unlikely]]
		{
			return min;
		}
		if (t >= 1) [[unlikely]]
		{
			return max;
		}
		return min + (max - min) * t;
	}

	/**
	* [EN]
	* Inverse of Lerp: returns the interpolation factor t such that
	* Lerp(min, max, t) == value. Returns 0 if min equals max.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Lerp の逆演算: Lerp(min, max, t) == value となる補間係数 t を返す。
	* min と max が等しい場合は 0 を返す。
	*/
	template<typename T, typename U>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Unlerp(T min, T max, U value)noexcept
	{
		const T range = max - min;
		if (range == 0) [[unlikely]]
		{
			return 0;
		}
		return (value - min) / range;
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the absolute value of a floating-point value.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 浮動小数点値の絶対値を返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Abs(T value)noexcept
	{
		return (value < 0) ? -value : value;
	}

	/**
	* [EN]
	* Returns the absolute value of an integral value.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 整数値の絶対値を返す。
	*/
	template<typename T>
		requires std::integral<T>
	[[nodiscard]] constexpr T Abs(T value)noexcept
	{
		return (value < 0) ? -value : value;
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Converts degrees to radians.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 度をラジアンへ変換する。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T ToRadians(T degrees)noexcept
	{
		return degrees * Pi<T>::ToRadian;
	}

	/**
	* [EN]
	* Converts radians to degrees.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ラジアンを度へ変換する。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T ToDegrees(T radians)noexcept
	{
		return radians * Pi<T>::ToDegree;
	}

	/**
	* [EN]
	* Returns the sine of radian.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* radian の正弦（サイン）を返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Sin(T radian)noexcept
	{
		return std::sin(radian);
	}

	/**
	* [EN]
	* Returns the cosine of radian.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* radian の余弦（コサイン）を返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Cos(T radian)noexcept
	{
		return std::cos(radian);
	}

	/**
	* [EN]
	* Returns the tangent of radian.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* radian の正接（タンジェント）を返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Tan(T radian)noexcept
	{
		return std::tan(radian);
	}

	/**
	* [EN]
	* Returns the arcsine of value, in radians.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value の逆正弦（アークサイン）をラジアンで返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Asin(T value)noexcept
	{
		return std::asin(value);
	}

	/**
	* [EN]
	* Returns the arccosine of value, in radians.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value の逆余弦（アークコサイン）をラジアンで返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Acos(T value)noexcept
	{
		return std::acos(value);
	}

	/**
	* [EN]
	* Returns the arctangent of value, in radians.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value の逆正接（アークタンジェント）をラジアンで返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Atan(T value)noexcept
	{
		return std::atan(value);
	}

	/**
	* [EN]
	* Returns the arctangent of y/x, in radians, using the signs of both
	* arguments to determine the correct quadrant.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* y/x の逆正接をラジアンで返す。両引数の符号を用いて正しい象限を
	* 判定する。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Atan2(T y, T x)noexcept
	{
		return std::atan2(y, x);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Rounds value toward zero, discarding its fractional part
	* (2.7 -> 2, -2.7 -> -2). NaN, infinities and values already too
	* large to carry a fractional part are returned unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を 0 方向へ丸め、小数部を捨てる（2.7 -> 2、-2.7 -> -2）。
	* NaN、無限大、および小数部を持てないほど大きい値はそのまま返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Trunc(T value)noexcept
	{
		/// [EN] NaN is the only value that compares unequal to itself; it has no integer part and cannot be converted to an integer type.
		/// [JP] NaN は自分自身と等しくならない唯一の値。整数部を持たず、整数型へ変換できない。
		if (value != value)
		{
			return value;
		}

		/// [EN] 2^(mantissa bits - 1): every floating-point value at or above this magnitude is already an integer (float: 2^23, double: 2^52), which also covers infinities and keeps the Int64 conversion below in range.
		/// [JP] 2^(仮数ビット数 - 1)。この大きさ以上の浮動小数点値は全て既に整数（float: 2^23、double: 2^52）。無限大もここに含まれ、下の Int64 への変換が範囲内に収まる。
		constexpr T limit = static_cast<T>(std::int64_t(1) << (std::numeric_limits<T>::digits - 1));
		if (Abs(value) >= limit)
		{
			return value;
		}

		/// [EN] Converting a floating-point value to an integer type truncates toward zero, which is exactly Trunc.
		/// [JP] 浮動小数点値から整数型への変換は 0 方向への切り捨てであり、そのまま Trunc になる。
		return static_cast<T>(static_cast<std::int64_t>(value));
	}

	/**
	* [EN]
	* Rounds value toward negative infinity: the greatest integer not
	* greater than value (2.7 -> 2, -2.7 -> -3).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を負の無限大方向へ丸める。value を超えない最大の整数を返す
	* （2.7 -> 2、-2.7 -> -3）。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Floor(T value)noexcept
	{
		/// [EN] value rounded toward zero; equals Floor for non-negative values and integers.
		/// [JP] 0 方向へ丸めた value。非負の値と整数では Floor と一致する。
		T truncated = Trunc(value);

		/// [EN] A negative value with a fractional part was rounded up by Trunc, so step one down.
		/// [JP] 小数部を持つ負の値は Trunc によって上へ丸められているため、1 つ下へずらす。
		if (value < truncated)
		{
			return truncated - static_cast<T>(1);
		}
		return truncated;
	}

	/**
	* [EN]
	* Rounds value toward positive infinity: the smallest integer not
	* less than value (2.3 -> 3, -2.3 -> -2).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を正の無限大方向へ丸める。value を下回らない最小の整数を返す
	* （2.3 -> 3、-2.3 -> -2）。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Ceil(T value)noexcept
	{
		/// [EN] value rounded toward zero; equals Ceil for non-positive values and integers.
		/// [JP] 0 方向へ丸めた value。非正の値と整数では Ceil と一致する。
		T truncated = Trunc(value);

		/// [EN] A positive value with a fractional part was rounded down by Trunc, so step one up.
		/// [JP] 小数部を持つ正の値は Trunc によって下へ丸められているため、1 つ上へずらす。
		if (value > truncated)
		{
			return truncated + static_cast<T>(1);
		}
		return truncated;
	}

	/**
	* [EN]
	* Rounds value to the nearest integer, with halfway cases rounded
	* away from zero (2.5 -> 3, -2.5 -> -3).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value を最も近い整数へ丸める。ちょうど中間の値は 0 から遠い方へ
	* 丸める（2.5 -> 3、-2.5 -> -3）。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Round(T value)noexcept
	{
		/// [EN] value rounded toward zero; value - truncated is the signed fractional part that was discarded.
		/// [JP] 0 方向へ丸めた value。value - truncated が捨てられた符号付きの小数部になる。
		T truncated = Trunc(value);

		/// [EN] The discarded fraction is computed exactly, so values just below one half stay below it; at one half or more, step one away from zero in the direction of value's sign.
		/// [JP] 捨てられた小数部は誤差なく求まるため、0.5 にわずかに満たない値は 0.5 未満のまま残る。0.5 以上なら、value の符号の向きへ 0 から 1 つ遠ざける。
		if (Abs(value - truncated) >= T(0.5))
		{
			return truncated + (value >= T(0) ? T(1) : T(-1));
		}
		return truncated;
	}

	/**
	* [EN]
	* Returns the fractional part of value measured from Floor, always in
	* [0, 1) (2.75 -> 0.75, -1.25 -> 0.75).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Floor を基準にした value の小数部を返す。結果は常に [0, 1)
	* （2.75 -> 0.75、-1.25 -> 0.75）。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Frac(T value)noexcept
	{
		return value - Floor(value);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Forward declaration of Exp2, defined further below. The real-exponent
	* Pow calls it, and calls with arguments of a built-in floating-point
	* type only find functions declared before the template definition.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 後方で定義する Exp2 の前方宣言。実数指数版の Pow から呼ばれる。
	* 組み込みの浮動小数点型を引数にした呼び出しは、テンプレート定義より
	* 前に宣言された関数しか見つけられない。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Exp2(T value)noexcept;

	/**
	* [EN]
	* Forward declaration of Log2, defined further below, for the same
	* reason as Exp2.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 後方で定義する Log2 の前方宣言。理由は Exp2 と同じ。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Log2(T value)noexcept;

	/**
	* [EN]
	* Raises base to an integer power by exponentiation by squaring:
	* base^n = (base^2)^(n/2) for even n, base * (base^2)^((n-1)/2) for
	* odd n, taking O(log n) multiplications. A negative exponent raises
	* 1/base to |exponent|, and any base to the power 0 is 1.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 繰り返し二乗法で base を整数乗する。n が偶数なら
	* base^n = (base^2)^(n/2)、奇数なら base * (base^2)^((n-1)/2) で、
	* 乗算は O(log n) 回。負の指数では 1/base を |exponent| 乗し、
	* 0 乗はどの base でも 1 になる。
	*/
	template<typename T, typename U>
		requires std::floating_point<T>&& std::integral<U>
	[[nodiscard]] constexpr T Pow(T base, U exponent)noexcept
	{
		/// [EN] Whether the exponent is negative, in which case the reciprocal of base is raised to |exponent|.
		/// [JP] 指数が負かどうか。負なら base の逆数を |exponent| 乗する。
		bool negative = exponent < U(0);

		/// [EN] |exponent| as an unsigned integer; subtracting from 0 in unsigned arithmetic also covers the most negative value, whose negation does not fit the signed type.
		/// [JP] |exponent| を符号なし整数で表したもの。符号なし演算で 0 から引くことで、符号付き型では符号反転が収まらない最小値も扱える。
		std::uint64_t remaining = negative ? std::uint64_t(0) - static_cast<std::uint64_t>(exponent) : static_cast<std::uint64_t>(exponent);

		/// [EN] base^(2^k) for the bit of remaining currently examined; starting from 1/base for a negative exponent keeps tiny results such as 2^-149 representable instead of overflowing the intermediate 2^149.
		/// [JP] remaining の現在見ているビットに対応する base^(2^k)。負の指数では 1/base から始めることで、2^-149 のような極小の結果を、途中の 2^149 をあふれさせずに表せる。
		T square = negative ? T(1) / base : base;

		/// [EN] Product of the powers selected by the set bits of the exponent so far.
		/// [JP] ここまでに指数の立っているビットで選ばれた累乗の積。
		T result = T(1);

		/// [EN] Walk the exponent's bits from lowest to highest, multiplying in base^(2^k) for every set bit k.
		/// [JP] 指数のビットを下位から上位へたどり、立っているビット k ごとに base^(2^k) を掛け合わせる。
		while (remaining > 0)
		{
			if ((remaining & 1) != 0)
			{
				result *= square;
			}
			square *= square;
			remaining >>= 1;
		}
		return result;
	}

	/**
	* [EN]
	* Raises base to a real power. Integer exponents use the
	* exponentiation-by-squaring overload, so negative bases work for
	* them; otherwise the result is 2^(exponent * log2(base)). A negative
	* base with a non-integer exponent has no real result and returns NaN;
	* a zero base returns 0 for a positive exponent and infinity for a
	* negative one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* base を実数乗する。整数の指数は繰り返し二乗法のオーバーロードで
	* 計算するため、負の base にも対応する。それ以外は
	* 2^(exponent * log2(base)) で求める。負の base を非整数乗した結果は
	* 実数にならないため NaN を返す。base が 0 なら、指数が正のとき 0、
	* 負のとき無限大を返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Pow(T base, T exponent)noexcept
	{
		/// [EN] NaN in either operand propagates.
		/// [JP] どちらかの引数が NaN なら NaN を伝播する。
		if (base != base || exponent != exponent)
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		/// [EN] Integer exponents within the Int64 range go through exponentiation by squaring, which is exact in its multiplications and defined for negative bases.
		/// [JP] Int64 の範囲に収まる整数の指数は繰り返し二乗法で計算する。乗算だけで済み、負の base にも定義される。
		if (Trunc(exponent) == exponent && Abs(exponent) < T(std::int64_t(1) << 62))
		{
			return Pow(base, static_cast<std::int64_t>(exponent));
		}

		/// [EN] A negative number raised to a non-integer power is not a real number.
		/// [JP] 負の数の非整数乗は実数にならない。
		if (base < T(0))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		/// [EN] log2(0) is -infinity, so 0 to a positive power is 0 and to a negative power is infinity.
		/// [JP] log2(0) は負の無限大なので、0 の正の乗は 0、負の乗は無限大になる。
		if (base == T(0))
		{
			return exponent > T(0) ? T(0) : std::numeric_limits<T>::infinity();
		}

		/// [EN] base^exponent = 2^(exponent * log2(base)).
		/// [JP] base^exponent = 2^(exponent * log2(base))。
		return Exp2(exponent * Log2(base));
	}

	/**
	* [EN]
	* Returns the square root of value by Newton's method,
	* y <- (y + value / y) / 2. Negative values return NaN; zero, infinity
	* and NaN are returned unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ニュートン法 y <- (y + value / y) / 2 で value の平方根を返す。
	* 負の値は NaN を返し、0・無限大・NaN はそのまま返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Sqrt(T value)noexcept
	{
		/// [EN] NaN, zero and infinity are their own square roots.
		/// [JP] NaN・0・無限大は、それ自身が平方根になる。
		if (value != value || value == T(0) || value == std::numeric_limits<T>::infinity())
		{
			return value;
		}

		/// [EN] Negative numbers have no real square root.
		/// [JP] 負の数は実数の平方根を持たない。
		if (value < T(0))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		/// [EN] max(value, 1) is never below the square root, and from above the Newton iteration decreases monotonically toward it.
		/// [JP] max(value, 1) は平方根を下回ることがなく、上側から始めたニュートン法は平方根へ向かって単調に減少する。
		T current = Max(value, T(1));

		/// [EN] Iterate until a step stops decreasing the estimate; at that point it has converged to the precision of T.
		/// [JP] 推定値が減らなくなるまで反復する。その時点で T の精度まで収束している。
		while (true)
		{
			T next = (current + value / current) / T(2);
			if (next >= current)
			{
				return current;
			}
			current = next;
		}
	}

	/**
	* [EN]
	* Returns 2^value. value is split into an integer part k = floor(value)
	* and a fraction f in [0, 1); 2^k comes from the integer Pow and
	* 2^f = e^(f * ln2) from its Taylor series. Results beyond the range
	* of T become infinity or 0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2^value を返す。value を整数部 k = floor(value) と [0, 1) の小数部 f
	* に分け、2^k は整数版の Pow で、2^f = e^(f * ln2) はそのテイラー
	* 級数で求める。T の範囲を超える結果は無限大または 0 になる。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Exp2(T value)noexcept
	{
		/// [EN] NaN propagates.
		/// [JP] NaN は NaN を伝播する。
		if (value != value)
		{
			return value;
		}

		/// [EN] 2^max_exponent is the first power of two above the largest finite value of T (2^128 for float), so the result overflows; +infinity lands here too.
		/// [JP] 2^max_exponent は T の最大有限値を超える最初の 2 の累乗（float では 2^128）なので、結果はあふれる。正の無限大もここに入る。
		if (value >= T(std::numeric_limits<T>::max_exponent))
		{
			return std::numeric_limits<T>::infinity();
		}

		/// [EN] 2^(min_exponent - digits) is the smallest subnormal of T (2^-149 for float); anything smaller rounds to 0, including -infinity.
		/// [JP] 2^(min_exponent - digits) は T の最小の非正規化数（float では 2^-149）。それより小さい値は 0 に丸まり、負の無限大もここに入る。
		if (value < T(std::numeric_limits<T>::min_exponent - std::numeric_limits<T>::digits))
		{
			return T(0);
		}

		/// [EN] Integer part k; flooring keeps k at most max_exponent - 1, so 2^k itself never overflows while 2^f stays in [1, 2).
		/// [JP] 整数部 k。切り捨てにより k は max_exponent - 1 以下に収まり、2^f が [1, 2) にある間 2^k 自体があふれることはない。
		T integer = Floor(value);

		/// [EN] 2^f = e^r with r = f * ln2 in [0, ln2).
		/// [JP] 2^f = e^r。r = f * ln2 は [0, ln2) に入る。
		T r = (value - integer) * std::numbers::ln2_v<T>;

		/// [EN] Current Taylor term r^n / n!.
		/// [JP] 現在のテイラー級数の項 r^n / n!。
		T term = T(1);

		/// [EN] Partial sum of e^r, starting from the n = 0 term.
		/// [JP] e^r の部分和。n = 0 の項から始める。
		T sum = T(1);

		/// [EN] n of the next term, held in T so that no integer type is needed.
		/// [JP] 次の項の n。整数型を使わずに済むよう T で持つ。
		T divisor = T(1);

		/// [EN] Add terms until one no longer changes the sum, i.e. the series has converged to the precision of T.
		/// [JP] 項を足しても和が変わらなくなるまで加算する。すなわち級数が T の精度まで収束するまで。
		while (true)
		{
			term *= r / divisor;
			T next = sum + term;
			if (next == sum)
			{
				break;
			}
			sum = next;
			divisor += T(1);
		}

		/// [EN] 2^value = 2^k * 2^f.
		/// [JP] 2^value = 2^k * 2^f。
		return Pow(T(2), static_cast<std::int64_t>(integer)) * sum;
	}

	/**
	* [EN]
	* Returns e^value, computed as 2^(value * log2(e)).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* e^value を返す。2^(value * log2(e)) として求める。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Exp(T value)noexcept
	{
		return Exp2(value * std::numbers::log2e_v<T>);
	}

	/**
	* [EN]
	* Returns the natural logarithm of value. value is split into
	* m * 2^k with m in [sqrt(1/2), sqrt(2)), and
	* ln(value) = k * ln2 + ln(m), where
	* ln(m) = 2 * (s + s^3/3 + s^5/5 + ...) with s = (m - 1) / (m + 1).
	* Negative values return NaN, 0 returns -infinity, and infinity and
	* NaN are returned unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value の自然対数を返す。value を m * 2^k（m は [sqrt(1/2), sqrt(2))）
	* に分け、ln(value) = k * ln2 + ln(m) とする。ここで
	* s = (m - 1) / (m + 1) として ln(m) = 2 * (s + s^3/3 + s^5/5 + ...)。
	* 負の値は NaN、0 は負の無限大を返し、無限大と NaN はそのまま返す。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Log(T value)noexcept
	{
		/// [EN] NaN propagates and ln(infinity) is infinity.
		/// [JP] NaN は NaN を伝播し、ln(無限大) は無限大になる。
		if (value != value || value == std::numeric_limits<T>::infinity())
		{
			return value;
		}

		/// [EN] Negative numbers have no real logarithm.
		/// [JP] 負の数は実数の対数を持たない。
		if (value < T(0))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		/// [EN] ln(x) diverges to -infinity as x approaches 0.
		/// [JP] ln(x) は x が 0 に近づくと負の無限大へ発散する。
		if (value == T(0))
		{
			return -std::numeric_limits<T>::infinity();
		}

		/// [EN] Mantissa m of value = m * 2^k, brought into [sqrt(1/2), sqrt(2)).
		/// [JP] value = m * 2^k の仮数 m。[sqrt(1/2), sqrt(2)) に収める。
		T mantissa = value;

		/// [EN] Exponent k of value = m * 2^k, held in T so that no integer type is needed.
		/// [JP] value = m * 2^k の指数 k。整数型を使わずに済むよう T で持つ。
		T exponent = T(0);

		/// [EN] Halving and doubling are exact in binary floating point, so the split introduces no error. Centring m on 1 instead of using [1, 2) keeps |s| at most about 0.172 rather than 1/3, roughly halving the number of series terms.
		/// [JP] 2 進浮動小数点では半分にする・倍にする操作は誤差なく行えるため、分解で誤差は生じない。m を [1, 2) ではなく 1 を中心とした範囲に収めることで、|s| が 1/3 ではなく約 0.172 以下になり、級数の項数がおよそ半分になる。
		while (mantissa >= std::numbers::sqrt2_v<T>)
		{
			mantissa /= T(2);
			exponent += T(1);
		}
		while (mantissa < std::numbers::sqrt2_v<T> / T(2))
		{
			mantissa *= T(2);
			exponent -= T(1);
		}

		/// [EN] s = (m - 1) / (m + 1), the variable of the atanh series ln(m) = 2 * atanh(s).
		/// [JP] s = (m - 1) / (m + 1)。atanh 級数 ln(m) = 2 * atanh(s) の変数。
		T s = (mantissa - T(1)) / (mantissa + T(1));

		/// [EN] s^2, the ratio between consecutive odd powers of s.
		/// [JP] s^2。連続する s の奇数乗どうしの比。
		T squared = s * s;

		/// [EN] Current odd power s^n.
		/// [JP] 現在の s の奇数乗 s^n。
		T term = s;

		/// [EN] Partial sum of s + s^3/3 + s^5/5 + ...
		/// [JP] s + s^3/3 + s^5/5 + ... の部分和。
		T sum = T(0);

		/// [EN] Odd denominator n of the current term.
		/// [JP] 現在の項の奇数の分母 n。
		T divisor = T(1);

		/// [EN] Add terms until one no longer changes the sum, i.e. the series has converged to the precision of T.
		/// [JP] 項を足しても和が変わらなくなるまで加算する。すなわち級数が T の精度まで収束するまで。
		while (true)
		{
			T next = sum + term / divisor;
			if (next == sum)
			{
				break;
			}
			sum = next;
			term *= squared;
			divisor += T(2);
		}

		/// [EN] ln(value) = k * ln2 + 2 * atanh(s).
		/// [JP] ln(value) = k * ln2 + 2 * atanh(s)。
		return exponent * std::numbers::ln2_v<T> + T(2) * sum;
	}

	/**
	* [EN]
	* Returns the base-2 logarithm of value, computed as
	* ln(value) * log2(e). Special values follow Log.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* value の 2 を底とする対数を返す。ln(value) * log2(e) として求める。
	* 特殊な値の扱いは Log に従う。
	*/
	template<typename T>
		requires std::floating_point<T>
	[[nodiscard]] constexpr T Log2(T value)noexcept
	{
		return Log(value) * std::numbers::log2e_v<T>;
	}
}
