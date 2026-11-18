#pragma once

namespace SeedCore
{
	struct Ivector2
	{
		Int32 x;
		Int32 y;

		constexpr Ivector2();
		constexpr Ivector2(Int32 x, Int32 y);
		constexpr explicit Ivector2(const Vector2& vector);

		constexpr explicit operator Vector2()const;

		constexpr Bool operator==(const Ivector2& vector)const;
		constexpr Bool operator!=(const Ivector2& vector)const;

		constexpr Ivector2& operator+=(const Ivector2& vector);
		constexpr Ivector2& operator-=(const Ivector2& vector);
		constexpr Ivector2& operator*=(const Ivector2& vector);
		constexpr Ivector2& operator/=(const Ivector2& vector);
		constexpr Ivector2& operator%=(const Ivector2& vector);
		constexpr Ivector2& operator&=(const Ivector2& vector);
		constexpr Ivector2& operator|=(const Ivector2& vector);
		constexpr Ivector2& operator^=(const Ivector2& vector);

		constexpr Ivector2& operator*=(Int32 scalar);
		constexpr Ivector2& operator/=(Int32 scalar);
		constexpr Ivector2& operator%=(Int32 scalar);
		constexpr Ivector2& operator<<=(Int32 shift);
		constexpr Ivector2& operator>>=(Int32 shift);

		constexpr Ivector2 operator+()const;
		constexpr Ivector2 operator-()const;
		constexpr Ivector2 operator~()const;

		constexpr Ivector2 operator+(const Ivector2& vector)const;
		constexpr Ivector2 operator-(const Ivector2& vector)const;
		constexpr Ivector2 operator*(const Ivector2& vector)const;
		constexpr Ivector2 operator/(const Ivector2& vector)const;
		constexpr Ivector2 operator%(const Ivector2& vector)const;
		constexpr Ivector2 operator&(const Ivector2& vector)const;
		constexpr Ivector2 operator|(const Ivector2& vector)const;
		constexpr Ivector2 operator^(const Ivector2& vector)const;

		constexpr Ivector2 operator*(Int32 scalar)const;
		constexpr Ivector2 operator/(Int32 scalar)const;
		constexpr Ivector2 operator%(Int32 scalar)const;
		constexpr Ivector2 operator<<(Int32 shift)const;
		constexpr Ivector2 operator>>(Int32 shift)const;

		friend constexpr Ivector2 operator*(Int32 scalar, const Ivector2& vector);

		NODISCARD constexpr Int64 LengthSq()const;

		NODISCARD constexpr Int64 Dot(const Ivector2& vector)const;

		static const Ivector2 Zero;
		static const Ivector2 One;
		static const Ivector2 UnitX;
		static const Ivector2 UnitY;
	};

	constexpr Ivector2::Ivector2() :x(0), y(0)
	{
		/// No Code
	}

	constexpr Ivector2::Ivector2(Int32 x, Int32 y) :x(x), y(y)
	{
		/// No Code
	}

	constexpr Ivector2::Ivector2(const Vector2& vector) :x(static_cast<Int32>(vector.x)), y(static_cast<Int32>(vector.y))
	{
		/// No Code
	}

	constexpr Ivector2::operator Vector2()const
	{
		return Vector2(static_cast<Float>(x), static_cast<Float>(y));
	}

	constexpr Bool Ivector2::operator==(const Ivector2& vector)const
	{
		return x == vector.x && y == vector.y;
	}

	constexpr Bool Ivector2::operator!=(const Ivector2& vector)const
	{
		return !(*this == vector);
	}

	constexpr Ivector2& Ivector2::operator+=(const Ivector2& vector)
	{
		x += vector.x;
		y += vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator-=(const Ivector2& vector)
	{
		x -= vector.x;
		y -= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator*=(const Ivector2& vector)
	{
		x *= vector.x;
		y *= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator/=(const Ivector2& vector)
	{
		x /= vector.x;
		y /= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator%=(const Ivector2& vector)
	{
		x %= vector.x;
		y %= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator&=(const Ivector2& vector)
	{
		x &= vector.x;
		y &= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator|=(const Ivector2& vector)
	{
		x |= vector.x;
		y |= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator^=(const Ivector2& vector)
	{
		x ^= vector.x;
		y ^= vector.y;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator*=(Int32 scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator/=(Int32 scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator%=(Int32 scalar)
	{
		x %= scalar;
		y %= scalar;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator<<=(Int32 shift)
	{
		x <<= shift;
		y <<= shift;
		return *this;
	}

	constexpr Ivector2& Ivector2::operator>>=(Int32 shift)
	{
		x >>= shift;
		y >>= shift;
		return *this;
	}

	constexpr Ivector2 Ivector2::operator+()const
	{
		return *this;
	}

	constexpr Ivector2 Ivector2::operator-()const
	{
		return Ivector2(-x, -y);
	}

	constexpr Ivector2 Ivector2::operator~()const
	{
		return Ivector2(~x, ~y);
	}

	constexpr Ivector2 Ivector2::operator+(const Ivector2& vector)const
	{
		return Ivector2(x + vector.x, y + vector.y);
	}

	constexpr Ivector2 Ivector2::operator-(const Ivector2& vector)const
	{
		return Ivector2(x - vector.x, y - vector.y);
	}

	constexpr Ivector2 Ivector2::operator*(const Ivector2& vector)const
	{
		return Ivector2(x * vector.x, y * vector.y);
	}

	constexpr Ivector2 Ivector2::operator/(const Ivector2& vector)const
	{
		return Ivector2(x / vector.x, y / vector.y);
	}

	constexpr Ivector2 Ivector2::operator%(const Ivector2& vector)const
	{
		return Ivector2(x % vector.x, y % vector.y);
	}

	constexpr Ivector2 Ivector2::operator&(const Ivector2& vector)const
	{
		return Ivector2(x & vector.x, y & vector.y);
	}

	constexpr Ivector2 Ivector2::operator|(const Ivector2& vector)const
	{
		return Ivector2(x | vector.x, y | vector.y);
	}

	constexpr Ivector2 Ivector2::operator^(const Ivector2& vector)const
	{
		return Ivector2(x ^ vector.x, y ^ vector.y);
	}

	constexpr Ivector2 Ivector2::operator*(Int32 scalar)const
	{
		return Ivector2(x * scalar, y * scalar);
	}

	constexpr Ivector2 Ivector2::operator/(Int32 scalar)const
	{
		return Ivector2(x / scalar, y / scalar);
	}

	constexpr Ivector2 Ivector2::operator%(Int32 scalar)const
	{
		return Ivector2(x % scalar, y % scalar);
	}

	constexpr Ivector2 Ivector2::operator<<(Int32 shift)const
	{
		return Ivector2(x << shift, y << shift);
	}

	constexpr Ivector2 Ivector2::operator>>(Int32 shift)const
	{
		return Ivector2(x >> shift, y >> shift);
	}

	constexpr Ivector2 operator*(Int32 scalar, const Ivector2& vector)
	{
		return Ivector2(scalar * vector.x, scalar * vector.y);
	}

	constexpr Int64 Ivector2::LengthSq()const
	{
		return static_cast<Int64>(x) * x + static_cast<Int64>(y) * y;
	}

	constexpr Int64 Ivector2::Dot(const Ivector2& vector)const
	{
		return static_cast<Int64>(x) * vector.x + static_cast<Int64>(y) * vector.y;
	}

	inline constexpr Ivector2 Ivector2::Zero = { 0, 0 };
	inline constexpr Ivector2 Ivector2::One = { 1, 1 };
	inline constexpr Ivector2 Ivector2::UnitX = { 1, 0 };
	inline constexpr Ivector2 Ivector2::UnitY = { 0, 1 };
}
