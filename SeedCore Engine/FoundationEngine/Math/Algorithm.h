#pragma once

namespace SeedCore
{
	template<typename T>
		requires std::floating_point<T>
	struct Pi
	{
		SC_CONSTEX T Value = std::numbers::pi_v<T>;

		SC_CONSTEX T Two = Value * T(2);

		SC_CONSTEX T Four = Value * T(4);

		SC_CONSTEX T Half = Value * T(0.5);

		SC_CONSTEX T Quarter = Value * T(0.25);

		SC_CONSTEX T Inverse = T(1) / Value;

		SC_CONSTEX T ToRadian = Value / T(180);

		SC_CONSTEX T ToDegree = T(180) / Value;
	};
}

namespace SeedCore
{
	template<typename T>
		requires std::totally_ordered<T>
	NODISCARD constexpr const T& Max(const T& a, const T& b)noexcept
	{
		return (a < b) ? b : a;
	}

	template<typename T, typename... Args>
		requires std::totally_ordered<T>
	NODISCARD constexpr std::common_type_t<T, Args...> Max(T first, Args... args)noexcept
	{
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

	template<typename T>
		requires std::totally_ordered<T>
	NODISCARD constexpr const T& Min(const T& a, const T& b)noexcept
	{
		return (b < a) ? b : a;
	}

	template<typename T, typename... Args>
		requires std::totally_ordered<T>
	NODISCARD constexpr std::common_type_t<T, Args...> Min(T first, Args... args) noexcept
	{
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
	template<typename T, typename U>
		requires std::totally_ordered<T>
	NODISCARD constexpr T Clamp(const U& value, const T& min, const T& max)noexcept
	{
		using ReturnType = std::common_type_t<T, U>;
		return Max(static_cast<ReturnType>(min), Min(static_cast<ReturnType>(value), static_cast<ReturnType>(max)));
	}

	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::floating_point<T>
	NODISCARD constexpr T Wrap(U value, T min, T max)noexcept
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

	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::integral<T>
	NODISCARD constexpr T Wrap(U value, T min, T max)noexcept
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

	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::floating_point<T>
	NODISCARD constexpr T Pingpong(U value, T min, T max)noexcept
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

	template<typename T, typename U>
		requires std::totally_ordered<T>&& std::integral<T>
	NODISCARD constexpr T Pingpong(U value, T min, T max)noexcept
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
	template<typename T, typename U>
		requires std::floating_point<T>
	NODISCARD constexpr T Lerp(T min, T max, U t)noexcept
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

	template<typename T, typename U>
		requires std::floating_point<T>
	NODISCARD constexpr T Unlerp(T min, T max, U value)noexcept
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
	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Abs(T value)noexcept
	{
		return (value < 0) ? -value : value;
	}

	template<typename T>
		requires std::integral<T>
	NODISCARD constexpr T Abs(T value)noexcept
	{
		return (value < 0) ? -value : value;
	}
}

namespace SeedCore
{
	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T ToRadians(T degrees)noexcept
	{
		return degrees * Pi<T>::ToRadian;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T ToDegrees(T radians)noexcept
	{
		return radians * Pi<T>::ToDegree;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Sin(T radian)noexcept
	{
		return std::sin(radian);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Cos(T radian)noexcept
	{
		return std::cos(radian);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Tan(T radian)noexcept
	{
		return std::tan(radian);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Asin(T value)noexcept
	{
		return std::asin(value);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Acos(T value)noexcept
	{
		return std::acos(value);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Atan(T value)noexcept
	{
		return std::atan(value);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Atan2(T y, T x)noexcept
	{
		return std::atan2(y, x);
	}
}

namespace SeedCore
{
	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Trunc(T value)noexcept
	{
		if (value != value)
		{
			return value;
		}

		constexpr T limit = static_cast<T>(std::int64_t(1) << (std::numeric_limits<T>::digits - 1));
		if (Abs(value) >= limit)
		{
			return value;
		}

		return static_cast<T>(static_cast<std::int64_t>(value));
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Floor(T value)noexcept
	{
		T truncated = Trunc(value);

		if (value < truncated)
		{
			return truncated - static_cast<T>(1);
		}
		return truncated;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Ceil(T value)noexcept
	{
		T truncated = Trunc(value);

		if (value > truncated)
		{
			return truncated + static_cast<T>(1);
		}
		return truncated;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Round(T value)noexcept
	{
		T truncated = Trunc(value);

		if (Abs(value - truncated) >= T(0.5))
		{
			return truncated + (value >= T(0) ? T(1) : T(-1));
		}
		return truncated;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Frac(T value)noexcept
	{
		return value - Floor(value);
	}
}

namespace SeedCore
{
	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Exp2(T value)noexcept;

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Log2(T value)noexcept;

	template<typename T, typename U>
		requires std::floating_point<T>&& std::integral<U>
	NODISCARD constexpr T Pow(T base, U exponent)noexcept
	{
		bool negative = exponent < U(0);

		std::uint64_t remaining = negative ? std::uint64_t(0) - static_cast<std::uint64_t>(exponent) : static_cast<std::uint64_t>(exponent);

		T square = negative ? T(1) / base : base;

		T result = T(1);

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

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Pow(T base, T exponent)noexcept
	{
		if (base != base || exponent != exponent)
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		if (Trunc(exponent) == exponent && Abs(exponent) < T(std::int64_t(1) << 62))
		{
			return Pow(base, static_cast<std::int64_t>(exponent));
		}

		if (base < T(0))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		if (base == T(0))
		{
			return exponent > T(0) ? T(0) : std::numeric_limits<T>::infinity();
		}

		return Exp2(exponent * Log2(base));
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Sqrt(T value)noexcept
	{
		if (value != value || value == T(0) || value == std::numeric_limits<T>::infinity())
		{
			return value;
		}

		if (value < T(0))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		T current = Max(value, T(1));

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

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Exp2(T value)noexcept
	{
		if (value != value)
		{
			return value;
		}

		if (value >= T(std::numeric_limits<T>::max_exponent))
		{
			return std::numeric_limits<T>::infinity();
		}

		if (value < T(std::numeric_limits<T>::min_exponent - std::numeric_limits<T>::digits))
		{
			return T(0);
		}

		T integer = Floor(value);

		T r = (value - integer) * std::numbers::ln2_v<T>;

		T term = T(1);

		T sum = T(1);

		T divisor = T(1);

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

		return Pow(T(2), static_cast<std::int64_t>(integer)) * sum;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Exp(T value)noexcept
	{
		return Exp2(value * std::numbers::log2e_v<T>);
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Log(T value)noexcept
	{
		if (value != value || value == std::numeric_limits<T>::infinity())
		{
			return value;
		}

		if (value < T(0))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}

		if (value == T(0))
		{
			return -std::numeric_limits<T>::infinity();
		}

		T mantissa = value;

		T exponent = T(0);

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

		T s = (mantissa - T(1)) / (mantissa + T(1));

		T squared = s * s;

		T term = s;

		T sum = T(0);

		T divisor = T(1);

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

		return exponent * std::numbers::ln2_v<T> + T(2) * sum;
	}

	template<typename T>
		requires std::floating_point<T>
	NODISCARD constexpr T Log2(T value)noexcept
	{
		return Log(value) * std::numbers::log2e_v<T>;
	}
}
