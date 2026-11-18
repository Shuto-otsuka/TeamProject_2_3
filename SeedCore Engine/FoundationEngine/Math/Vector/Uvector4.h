#pragma once

namespace SeedCore
{
	struct Uvector4
	{
		union
		{
			struct
			{
				Uint32 x;
				Uint32 y;
				Uint32 z;
				Uint32 w;
			};
			struct
			{
				Uvector2 xy;
			};
			struct
			{
				Uint32 : 32;
				Uvector2 yz;
			};
			struct
			{
				Uint32 : 32;
				Uint32 : 32;
				Uvector2 zw;
			};
			struct
			{
				Uvector3 xyz;
			};
			struct
			{
				Uint32 : 32;
				Uvector3 yzw;
			};
		};

		constexpr Uvector4();
		constexpr Uvector4(Uint32 x, Uint32 y, Uint32 z, Uint32 w);
		constexpr explicit Uvector4(const Vector4& vector);
		constexpr explicit Uvector4(const Ivector4& vector);

		constexpr explicit operator Vector4()const;
		constexpr explicit operator Ivector4()const;

		constexpr Bool operator==(const Uvector4& vector)const;
		constexpr Bool operator!=(const Uvector4& vector)const;

		constexpr Uvector4& operator+=(const Uvector4& vector);
		constexpr Uvector4& operator-=(const Uvector4& vector);
		constexpr Uvector4& operator*=(const Uvector4& vector);
		constexpr Uvector4& operator/=(const Uvector4& vector);
		constexpr Uvector4& operator%=(const Uvector4& vector);
		constexpr Uvector4& operator&=(const Uvector4& vector);
		constexpr Uvector4& operator|=(const Uvector4& vector);
		constexpr Uvector4& operator^=(const Uvector4& vector);

		constexpr Uvector4& operator*=(Uint32 scalar);
		constexpr Uvector4& operator/=(Uint32 scalar);
		constexpr Uvector4& operator%=(Uint32 scalar);
		constexpr Uvector4& operator<<=(Int32 shift);
		constexpr Uvector4& operator>>=(Int32 shift);

		constexpr Uvector4 operator+()const;
		constexpr Uvector4 operator~()const;

		constexpr Uvector4 operator+(const Uvector4& vector)const;
		constexpr Uvector4 operator-(const Uvector4& vector)const;
		constexpr Uvector4 operator*(const Uvector4& vector)const;
		constexpr Uvector4 operator/(const Uvector4& vector)const;
		constexpr Uvector4 operator%(const Uvector4& vector)const;
		constexpr Uvector4 operator&(const Uvector4& vector)const;
		constexpr Uvector4 operator|(const Uvector4& vector)const;
		constexpr Uvector4 operator^(const Uvector4& vector)const;

		constexpr Uvector4 operator*(Uint32 scalar)const;
		constexpr Uvector4 operator/(Uint32 scalar)const;
		constexpr Uvector4 operator%(Uint32 scalar)const;
		constexpr Uvector4 operator<<(Int32 shift)const;
		constexpr Uvector4 operator>>(Int32 shift)const;

		friend constexpr Uvector4 operator*(Uint32 scalar, const Uvector4& vector);

		NODISCARD constexpr Uint64 LengthSq()const;

		NODISCARD constexpr Uint64 Dot(const Uvector4& vector)const;

		static const Uvector4 Zero;
		static const Uvector4 One;
		static const Uvector4 UnitX;
		static const Uvector4 UnitY;
		static const Uvector4 UnitZ;
		static const Uvector4 UnitW;
	};

	constexpr Uvector4::Uvector4() :x(0), y(0), z(0), w(0)
	{
		/// No Code
	}

	constexpr Uvector4::Uvector4(Uint32 x, Uint32 y, Uint32 z, Uint32 w) :x(x), y(y), z(z), w(w)
	{
		/// No Code
	}

	constexpr Uvector4::Uvector4(const Vector4& vector) :x(static_cast<Uint32>(vector.x)), y(static_cast<Uint32>(vector.y)), z(static_cast<Uint32>(vector.z)), w(static_cast<Uint32>(vector.w))
	{
		/// No Code
	}

	constexpr Uvector4::Uvector4(const Ivector4& vector) :x(static_cast<Uint32>(vector.x)), y(static_cast<Uint32>(vector.y)), z(static_cast<Uint32>(vector.z)), w(static_cast<Uint32>(vector.w))
	{
		/// No Code
	}

	constexpr Uvector4::operator Vector4()const
	{
		return Vector4(static_cast<Float>(x), static_cast<Float>(y), static_cast<Float>(z), static_cast<Float>(w));
	}

	constexpr Uvector4::operator Ivector4()const
	{
		return Ivector4(static_cast<Int32>(x), static_cast<Int32>(y), static_cast<Int32>(z), static_cast<Int32>(w));
	}

	constexpr Bool Uvector4::operator==(const Uvector4& vector)const
	{
		return x == vector.x && y == vector.y && z == vector.z && w == vector.w;
	}

	constexpr Bool Uvector4::operator!=(const Uvector4& vector)const
	{
		return !(*this == vector);
	}

	constexpr Uvector4& Uvector4::operator+=(const Uvector4& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		w += vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator-=(const Uvector4& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		w -= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator*=(const Uvector4& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		w *= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator/=(const Uvector4& vector)
	{
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		w /= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator%=(const Uvector4& vector)
	{
		x %= vector.x;
		y %= vector.y;
		z %= vector.z;
		w %= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator&=(const Uvector4& vector)
	{
		x &= vector.x;
		y &= vector.y;
		z &= vector.z;
		w &= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator|=(const Uvector4& vector)
	{
		x |= vector.x;
		y |= vector.y;
		z |= vector.z;
		w |= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator^=(const Uvector4& vector)
	{
		x ^= vector.x;
		y ^= vector.y;
		z ^= vector.z;
		w ^= vector.w;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator*=(Uint32 scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator/=(Uint32 scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		w /= scalar;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator%=(Uint32 scalar)
	{
		x %= scalar;
		y %= scalar;
		z %= scalar;
		w %= scalar;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator<<=(Int32 shift)
	{
		x <<= shift;
		y <<= shift;
		z <<= shift;
		w <<= shift;
		return *this;
	}

	constexpr Uvector4& Uvector4::operator>>=(Int32 shift)
	{
		x >>= shift;
		y >>= shift;
		z >>= shift;
		w >>= shift;
		return *this;
	}

	constexpr Uvector4 Uvector4::operator+()const
	{
		return *this;
	}

	constexpr Uvector4 Uvector4::operator~()const
	{
		return Uvector4(~x, ~y, ~z, ~w);
	}

	constexpr Uvector4 Uvector4::operator+(const Uvector4& vector)const
	{
		return Uvector4(x + vector.x, y + vector.y, z + vector.z, w + vector.w);
	}

	constexpr Uvector4 Uvector4::operator-(const Uvector4& vector)const
	{
		return Uvector4(x - vector.x, y - vector.y, z - vector.z, w - vector.w);
	}

	constexpr Uvector4 Uvector4::operator*(const Uvector4& vector)const
	{
		return Uvector4(x * vector.x, y * vector.y, z * vector.z, w * vector.w);
	}

	constexpr Uvector4 Uvector4::operator/(const Uvector4& vector)const
	{
		return Uvector4(x / vector.x, y / vector.y, z / vector.z, w / vector.w);
	}

	constexpr Uvector4 Uvector4::operator%(const Uvector4& vector)const
	{
		return Uvector4(x % vector.x, y % vector.y, z % vector.z, w % vector.w);
	}

	constexpr Uvector4 Uvector4::operator&(const Uvector4& vector)const
	{
		return Uvector4(x & vector.x, y & vector.y, z & vector.z, w & vector.w);
	}

	constexpr Uvector4 Uvector4::operator|(const Uvector4& vector)const
	{
		return Uvector4(x | vector.x, y | vector.y, z | vector.z, w | vector.w);
	}

	constexpr Uvector4 Uvector4::operator^(const Uvector4& vector)const
	{
		return Uvector4(x ^ vector.x, y ^ vector.y, z ^ vector.z, w ^ vector.w);
	}

	constexpr Uvector4 Uvector4::operator*(Uint32 scalar)const
	{
		return Uvector4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	constexpr Uvector4 Uvector4::operator/(Uint32 scalar)const
	{
		return Uvector4(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	constexpr Uvector4 Uvector4::operator%(Uint32 scalar)const
	{
		return Uvector4(x % scalar, y % scalar, z % scalar, w % scalar);
	}

	constexpr Uvector4 Uvector4::operator<<(Int32 shift)const
	{
		return Uvector4(x << shift, y << shift, z << shift, w << shift);
	}

	constexpr Uvector4 Uvector4::operator>>(Int32 shift)const
	{
		return Uvector4(x >> shift, y >> shift, z >> shift, w >> shift);
	}

	constexpr Uvector4 operator*(Uint32 scalar, const Uvector4& vector)
	{
		return Uvector4(scalar * vector.x, scalar * vector.y, scalar * vector.z, scalar * vector.w);
	}

	constexpr Uint64 Uvector4::LengthSq()const
	{
		return static_cast<Uint64>(x) * x + static_cast<Uint64>(y) * y + static_cast<Uint64>(z) * z + static_cast<Uint64>(w) * w;
	}

	constexpr Uint64 Uvector4::Dot(const Uvector4& vector)const
	{
		return static_cast<Uint64>(x) * vector.x + static_cast<Uint64>(y) * vector.y + static_cast<Uint64>(z) * vector.z + static_cast<Uint64>(w) * vector.w;
	}

	inline constexpr Uvector4 Uvector4::Zero = { 0, 0, 0, 0 };
	inline constexpr Uvector4 Uvector4::One = { 1, 1, 1, 1 };
	inline constexpr Uvector4 Uvector4::UnitX = { 1, 0, 0, 0 };
	inline constexpr Uvector4 Uvector4::UnitY = { 0, 1, 0, 0 };
	inline constexpr Uvector4 Uvector4::UnitZ = { 0, 0, 1, 0 };
	inline constexpr Uvector4 Uvector4::UnitW = { 0, 0, 0, 1 };
}
