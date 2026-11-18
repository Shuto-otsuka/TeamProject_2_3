#pragma once

namespace SeedCore
{
	struct Ivector4
	{
		union
		{
			struct
			{
				Int32 x;
				Int32 y;
				Int32 z;
				Int32 w;
			};
			struct
			{
				Ivector2 xy;
			};
			struct
			{
				Uint32 : 32;
				Ivector2 yz;
			};
			struct
			{
				Uint32 : 32;
				Uint32 : 32;
				Ivector2 zw;
			};
			struct
			{
				Ivector3 xyz;
			};
			struct
			{
				Uint32 : 32;
				Ivector3 yzw;
			};
		};

		constexpr Ivector4();
		constexpr Ivector4(Int32 x, Int32 y, Int32 z, Int32 w);
		constexpr explicit Ivector4(const Vector4& vector);

		constexpr explicit operator Vector4()const;

		constexpr Bool operator==(const Ivector4& vector)const;
		constexpr Bool operator!=(const Ivector4& vector)const;

		constexpr Ivector4& operator+=(const Ivector4& vector);
		constexpr Ivector4& operator-=(const Ivector4& vector);
		constexpr Ivector4& operator*=(const Ivector4& vector);
		constexpr Ivector4& operator/=(const Ivector4& vector);
		constexpr Ivector4& operator%=(const Ivector4& vector);
		constexpr Ivector4& operator&=(const Ivector4& vector);
		constexpr Ivector4& operator|=(const Ivector4& vector);
		constexpr Ivector4& operator^=(const Ivector4& vector);

		constexpr Ivector4& operator*=(Int32 scalar);
		constexpr Ivector4& operator/=(Int32 scalar);
		constexpr Ivector4& operator%=(Int32 scalar);
		constexpr Ivector4& operator<<=(Int32 shift);
		constexpr Ivector4& operator>>=(Int32 shift);

		constexpr Ivector4 operator+()const;
		constexpr Ivector4 operator-()const;
		constexpr Ivector4 operator~()const;

		constexpr Ivector4 operator+(const Ivector4& vector)const;
		constexpr Ivector4 operator-(const Ivector4& vector)const;
		constexpr Ivector4 operator*(const Ivector4& vector)const;
		constexpr Ivector4 operator/(const Ivector4& vector)const;
		constexpr Ivector4 operator%(const Ivector4& vector)const;
		constexpr Ivector4 operator&(const Ivector4& vector)const;
		constexpr Ivector4 operator|(const Ivector4& vector)const;
		constexpr Ivector4 operator^(const Ivector4& vector)const;

		constexpr Ivector4 operator*(Int32 scalar)const;
		constexpr Ivector4 operator/(Int32 scalar)const;
		constexpr Ivector4 operator%(Int32 scalar)const;
		constexpr Ivector4 operator<<(Int32 shift)const;
		constexpr Ivector4 operator>>(Int32 shift)const;

		friend constexpr Ivector4 operator*(Int32 scalar, const Ivector4& vector);

		NODISCARD constexpr Int64 LengthSq()const;

		NODISCARD constexpr Int64 Dot(const Ivector4& vector)const;

		static const Ivector4 Zero;
		static const Ivector4 One;
		static const Ivector4 UnitX;
		static const Ivector4 UnitY;
		static const Ivector4 UnitZ;
		static const Ivector4 UnitW;
	};

	constexpr Ivector4::Ivector4() :x(0), y(0), z(0), w(0)
	{
		/// No Code
	}

	constexpr Ivector4::Ivector4(Int32 x, Int32 y, Int32 z, Int32 w) :x(x), y(y), z(z), w(w)
	{
		/// No Code
	}

	constexpr Ivector4::Ivector4(const Vector4& vector) :x(static_cast<Int32>(vector.x)), y(static_cast<Int32>(vector.y)), z(static_cast<Int32>(vector.z)), w(static_cast<Int32>(vector.w))
	{
		/// No Code
	}

	constexpr Ivector4::operator Vector4()const
	{
		return Vector4(static_cast<Float>(x), static_cast<Float>(y), static_cast<Float>(z), static_cast<Float>(w));
	}

	constexpr Bool Ivector4::operator==(const Ivector4& vector)const
	{
		return x == vector.x && y == vector.y && z == vector.z && w == vector.w;
	}

	constexpr Bool Ivector4::operator!=(const Ivector4& vector)const
	{
		return !(*this == vector);
	}

	constexpr Ivector4& Ivector4::operator+=(const Ivector4& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		w += vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator-=(const Ivector4& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		w -= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator*=(const Ivector4& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		w *= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator/=(const Ivector4& vector)
	{
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		w /= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator%=(const Ivector4& vector)
	{
		x %= vector.x;
		y %= vector.y;
		z %= vector.z;
		w %= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator&=(const Ivector4& vector)
	{
		x &= vector.x;
		y &= vector.y;
		z &= vector.z;
		w &= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator|=(const Ivector4& vector)
	{
		x |= vector.x;
		y |= vector.y;
		z |= vector.z;
		w |= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator^=(const Ivector4& vector)
	{
		x ^= vector.x;
		y ^= vector.y;
		z ^= vector.z;
		w ^= vector.w;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator*=(Int32 scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator/=(Int32 scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		w /= scalar;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator%=(Int32 scalar)
	{
		x %= scalar;
		y %= scalar;
		z %= scalar;
		w %= scalar;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator<<=(Int32 shift)
	{
		x <<= shift;
		y <<= shift;
		z <<= shift;
		w <<= shift;
		return *this;
	}

	constexpr Ivector4& Ivector4::operator>>=(Int32 shift)
	{
		x >>= shift;
		y >>= shift;
		z >>= shift;
		w >>= shift;
		return *this;
	}

	constexpr Ivector4 Ivector4::operator+()const
	{
		return *this;
	}

	constexpr Ivector4 Ivector4::operator-()const
	{
		return Ivector4(-x, -y, -z, -w);
	}

	constexpr Ivector4 Ivector4::operator~()const
	{
		return Ivector4(~x, ~y, ~z, ~w);
	}

	constexpr Ivector4 Ivector4::operator+(const Ivector4& vector)const
	{
		return Ivector4(x + vector.x, y + vector.y, z + vector.z, w + vector.w);
	}

	constexpr Ivector4 Ivector4::operator-(const Ivector4& vector)const
	{
		return Ivector4(x - vector.x, y - vector.y, z - vector.z, w - vector.w);
	}

	constexpr Ivector4 Ivector4::operator*(const Ivector4& vector)const
	{
		return Ivector4(x * vector.x, y * vector.y, z * vector.z, w * vector.w);
	}

	constexpr Ivector4 Ivector4::operator/(const Ivector4& vector)const
	{
		return Ivector4(x / vector.x, y / vector.y, z / vector.z, w / vector.w);
	}

	constexpr Ivector4 Ivector4::operator%(const Ivector4& vector)const
	{
		return Ivector4(x % vector.x, y % vector.y, z % vector.z, w % vector.w);
	}

	constexpr Ivector4 Ivector4::operator&(const Ivector4& vector)const
	{
		return Ivector4(x & vector.x, y & vector.y, z & vector.z, w & vector.w);
	}

	constexpr Ivector4 Ivector4::operator|(const Ivector4& vector)const
	{
		return Ivector4(x | vector.x, y | vector.y, z | vector.z, w | vector.w);
	}

	constexpr Ivector4 Ivector4::operator^(const Ivector4& vector)const
	{
		return Ivector4(x ^ vector.x, y ^ vector.y, z ^ vector.z, w ^ vector.w);
	}

	constexpr Ivector4 Ivector4::operator*(Int32 scalar)const
	{
		return Ivector4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	constexpr Ivector4 Ivector4::operator/(Int32 scalar)const
	{
		return Ivector4(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	constexpr Ivector4 Ivector4::operator%(Int32 scalar)const
	{
		return Ivector4(x % scalar, y % scalar, z % scalar, w % scalar);
	}

	constexpr Ivector4 Ivector4::operator<<(Int32 shift)const
	{
		return Ivector4(x << shift, y << shift, z << shift, w << shift);
	}

	constexpr Ivector4 Ivector4::operator>>(Int32 shift)const
	{
		return Ivector4(x >> shift, y >> shift, z >> shift, w >> shift);
	}

	constexpr Ivector4 operator*(Int32 scalar, const Ivector4& vector)
	{
		return Ivector4(scalar * vector.x, scalar * vector.y, scalar * vector.z, scalar * vector.w);
	}

	constexpr Int64 Ivector4::LengthSq()const
	{
		return static_cast<Int64>(x) * x + static_cast<Int64>(y) * y + static_cast<Int64>(z) * z + static_cast<Int64>(w) * w;
	}

	constexpr Int64 Ivector4::Dot(const Ivector4& vector)const
	{
		return static_cast<Int64>(x) * vector.x + static_cast<Int64>(y) * vector.y + static_cast<Int64>(z) * vector.z + static_cast<Int64>(w) * vector.w;
	}

	inline constexpr Ivector4 Ivector4::Zero = { 0, 0, 0, 0 };
	inline constexpr Ivector4 Ivector4::One = { 1, 1, 1, 1 };
	inline constexpr Ivector4 Ivector4::UnitX = { 1, 0, 0, 0 };
	inline constexpr Ivector4 Ivector4::UnitY = { 0, 1, 0, 0 };
	inline constexpr Ivector4 Ivector4::UnitZ = { 0, 0, 1, 0 };
	inline constexpr Ivector4 Ivector4::UnitW = { 0, 0, 0, 1 };
}
