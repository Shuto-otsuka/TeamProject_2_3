#pragma once

namespace SeedCore
{
	struct Uvector2
	{
		Uint32 x;
		Uint32 y;

		constexpr Uvector2();
		constexpr Uvector2(Uint32 x, Uint32 y);
		constexpr explicit Uvector2(const Vector2& vector);
		constexpr explicit Uvector2(const Ivector2& vector);

		constexpr explicit operator Vector2()const;
		constexpr explicit operator Ivector2()const;

		constexpr Bool operator==(const Uvector2& vector)const;
		constexpr Bool operator!=(const Uvector2& vector)const;

		constexpr Uvector2& operator+=(const Uvector2& vector);
		constexpr Uvector2& operator-=(const Uvector2& vector);
		constexpr Uvector2& operator*=(const Uvector2& vector);
		constexpr Uvector2& operator/=(const Uvector2& vector);
		constexpr Uvector2& operator%=(const Uvector2& vector);
		constexpr Uvector2& operator&=(const Uvector2& vector);
		constexpr Uvector2& operator|=(const Uvector2& vector);
		constexpr Uvector2& operator^=(const Uvector2& vector);

		constexpr Uvector2& operator*=(Uint32 scalar);
		constexpr Uvector2& operator/=(Uint32 scalar);
		constexpr Uvector2& operator%=(Uint32 scalar);
		constexpr Uvector2& operator<<=(Int32 shift);
		constexpr Uvector2& operator>>=(Int32 shift);

		constexpr Uvector2 operator+()const;
		constexpr Uvector2 operator~()const;

		constexpr Uvector2 operator+(const Uvector2& vector)const;
		constexpr Uvector2 operator-(const Uvector2& vector)const;
		constexpr Uvector2 operator*(const Uvector2& vector)const;
		constexpr Uvector2 operator/(const Uvector2& vector)const;
		constexpr Uvector2 operator%(const Uvector2& vector)const;
		constexpr Uvector2 operator&(const Uvector2& vector)const;
		constexpr Uvector2 operator|(const Uvector2& vector)const;
		constexpr Uvector2 operator^(const Uvector2& vector)const;

		constexpr Uvector2 operator*(Uint32 scalar)const;
		constexpr Uvector2 operator/(Uint32 scalar)const;
		constexpr Uvector2 operator%(Uint32 scalar)const;
		constexpr Uvector2 operator<<(Int32 shift)const;
		constexpr Uvector2 operator>>(Int32 shift)const;

		friend constexpr Uvector2 operator*(Uint32 scalar, const Uvector2& vector);

		NODISCARD constexpr Uint64 LengthSq()const;

		NODISCARD constexpr Uint64 Dot(const Uvector2& vector)const;

		static const Uvector2 Zero;
		static const Uvector2 One;
		static const Uvector2 UnitX;
		static const Uvector2 UnitY;
	};

	constexpr Uvector2::Uvector2() :x(0), y(0)
	{
		/// No Code
	}

	constexpr Uvector2::Uvector2(Uint32 x, Uint32 y) :x(x), y(y)
	{
		/// No Code
	}

	constexpr Uvector2::Uvector2(const Vector2& vector) :x(static_cast<Uint32>(vector.x)), y(static_cast<Uint32>(vector.y))
	{
		/// No Code
	}

	constexpr Uvector2::Uvector2(const Ivector2& vector) :x(static_cast<Uint32>(vector.x)), y(static_cast<Uint32>(vector.y))
	{
		/// No Code
	}

	constexpr Uvector2::operator Vector2()const
	{
		return Vector2(static_cast<Float>(x), static_cast<Float>(y));
	}

	constexpr Uvector2::operator Ivector2()const
	{
		return Ivector2(static_cast<Int32>(x), static_cast<Int32>(y));
	}

	constexpr Bool Uvector2::operator==(const Uvector2& vector)const
	{
		return x == vector.x && y == vector.y;
	}

	constexpr Bool Uvector2::operator!=(const Uvector2& vector)const
	{
		return !(*this == vector);
	}

	constexpr Uvector2& Uvector2::operator+=(const Uvector2& vector)
	{
		x += vector.x;
		y += vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator-=(const Uvector2& vector)
	{
		x -= vector.x;
		y -= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator*=(const Uvector2& vector)
	{
		x *= vector.x;
		y *= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator/=(const Uvector2& vector)
	{
		x /= vector.x;
		y /= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator%=(const Uvector2& vector)
	{
		x %= vector.x;
		y %= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator&=(const Uvector2& vector)
	{
		x &= vector.x;
		y &= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator|=(const Uvector2& vector)
	{
		x |= vector.x;
		y |= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator^=(const Uvector2& vector)
	{
		x ^= vector.x;
		y ^= vector.y;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator*=(Uint32 scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator/=(Uint32 scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator%=(Uint32 scalar)
	{
		x %= scalar;
		y %= scalar;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator<<=(Int32 shift)
	{
		x <<= shift;
		y <<= shift;
		return *this;
	}

	constexpr Uvector2& Uvector2::operator>>=(Int32 shift)
	{
		x >>= shift;
		y >>= shift;
		return *this;
	}

	constexpr Uvector2 Uvector2::operator+()const
	{
		return *this;
	}

	constexpr Uvector2 Uvector2::operator~()const
	{
		return Uvector2(~x, ~y);
	}

	constexpr Uvector2 Uvector2::operator+(const Uvector2& vector)const
	{
		return Uvector2(x + vector.x, y + vector.y);
	}

	constexpr Uvector2 Uvector2::operator-(const Uvector2& vector)const
	{
		return Uvector2(x - vector.x, y - vector.y);
	}

	constexpr Uvector2 Uvector2::operator*(const Uvector2& vector)const
	{
		return Uvector2(x * vector.x, y * vector.y);
	}

	constexpr Uvector2 Uvector2::operator/(const Uvector2& vector)const
	{
		return Uvector2(x / vector.x, y / vector.y);
	}

	constexpr Uvector2 Uvector2::operator%(const Uvector2& vector)const
	{
		return Uvector2(x % vector.x, y % vector.y);
	}

	constexpr Uvector2 Uvector2::operator&(const Uvector2& vector)const
	{
		return Uvector2(x & vector.x, y & vector.y);
	}

	constexpr Uvector2 Uvector2::operator|(const Uvector2& vector)const
	{
		return Uvector2(x | vector.x, y | vector.y);
	}

	constexpr Uvector2 Uvector2::operator^(const Uvector2& vector)const
	{
		return Uvector2(x ^ vector.x, y ^ vector.y);
	}

	constexpr Uvector2 Uvector2::operator*(Uint32 scalar)const
	{
		return Uvector2(x * scalar, y * scalar);
	}

	constexpr Uvector2 Uvector2::operator/(Uint32 scalar)const
	{
		return Uvector2(x / scalar, y / scalar);
	}

	constexpr Uvector2 Uvector2::operator%(Uint32 scalar)const
	{
		return Uvector2(x % scalar, y % scalar);
	}

	constexpr Uvector2 Uvector2::operator<<(Int32 shift)const
	{
		return Uvector2(x << shift, y << shift);
	}

	constexpr Uvector2 Uvector2::operator>>(Int32 shift)const
	{
		return Uvector2(x >> shift, y >> shift);
	}

	constexpr Uvector2 operator*(Uint32 scalar, const Uvector2& vector)
	{
		return Uvector2(scalar * vector.x, scalar * vector.y);
	}

	constexpr Uint64 Uvector2::LengthSq()const
	{
		return static_cast<Uint64>(x) * x + static_cast<Uint64>(y) * y;
	}

	constexpr Uint64 Uvector2::Dot(const Uvector2& vector)const
	{
		return static_cast<Uint64>(x) * vector.x + static_cast<Uint64>(y) * vector.y;
	}

	inline constexpr Uvector2 Uvector2::Zero = { 0, 0 };
	inline constexpr Uvector2 Uvector2::One = { 1, 1 };
	inline constexpr Uvector2 Uvector2::UnitX = { 1, 0 };
	inline constexpr Uvector2 Uvector2::UnitY = { 0, 1 };
}
