#pragma once

namespace SeedCore
{
	struct Uvector3
	{
		union
		{
			struct
			{
				Uint32 x;
				Uint32 y;
				Uint32 z;
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
		};

		constexpr Uvector3();
		constexpr Uvector3(Uint32 x, Uint32 y, Uint32 z);
		constexpr explicit Uvector3(const Vector3& vector);
		constexpr explicit Uvector3(const Ivector3& vector);

		constexpr explicit operator Vector3()const;
		constexpr explicit operator Ivector3()const;

		constexpr Bool operator==(const Uvector3& vector)const;
		constexpr Bool operator!=(const Uvector3& vector)const;

		constexpr Uvector3& operator+=(const Uvector3& vector);
		constexpr Uvector3& operator-=(const Uvector3& vector);
		constexpr Uvector3& operator*=(const Uvector3& vector);
		constexpr Uvector3& operator/=(const Uvector3& vector);
		constexpr Uvector3& operator%=(const Uvector3& vector);
		constexpr Uvector3& operator&=(const Uvector3& vector);
		constexpr Uvector3& operator|=(const Uvector3& vector);
		constexpr Uvector3& operator^=(const Uvector3& vector);

		constexpr Uvector3& operator*=(Uint32 scalar);
		constexpr Uvector3& operator/=(Uint32 scalar);
		constexpr Uvector3& operator%=(Uint32 scalar);
		constexpr Uvector3& operator<<=(Int32 shift);
		constexpr Uvector3& operator>>=(Int32 shift);

		constexpr Uvector3 operator+()const;
		constexpr Uvector3 operator~()const;

		constexpr Uvector3 operator+(const Uvector3& vector)const;
		constexpr Uvector3 operator-(const Uvector3& vector)const;
		constexpr Uvector3 operator*(const Uvector3& vector)const;
		constexpr Uvector3 operator/(const Uvector3& vector)const;
		constexpr Uvector3 operator%(const Uvector3& vector)const;
		constexpr Uvector3 operator&(const Uvector3& vector)const;
		constexpr Uvector3 operator|(const Uvector3& vector)const;
		constexpr Uvector3 operator^(const Uvector3& vector)const;

		constexpr Uvector3 operator*(Uint32 scalar)const;
		constexpr Uvector3 operator/(Uint32 scalar)const;
		constexpr Uvector3 operator%(Uint32 scalar)const;
		constexpr Uvector3 operator<<(Int32 shift)const;
		constexpr Uvector3 operator>>(Int32 shift)const;

		friend constexpr Uvector3 operator*(Uint32 scalar, const Uvector3& vector);

		NODISCARD constexpr Uint64 LengthSq()const;

		NODISCARD constexpr Uint64 Dot(const Uvector3& vector)const;

		static const Uvector3 Zero;
		static const Uvector3 One;
		static const Uvector3 UnitX;
		static const Uvector3 UnitY;
		static const Uvector3 UnitZ;
	};

	constexpr Uvector3::Uvector3() :x(0), y(0), z(0)
	{
		/// No Code
	}

	constexpr Uvector3::Uvector3(Uint32 x, Uint32 y, Uint32 z) :x(x), y(y), z(z)
	{
		/// No Code
	}

	constexpr Uvector3::Uvector3(const Vector3& vector) :x(static_cast<Uint32>(vector.x)), y(static_cast<Uint32>(vector.y)), z(static_cast<Uint32>(vector.z))
	{
		/// No Code
	}

	constexpr Uvector3::Uvector3(const Ivector3& vector) :x(static_cast<Uint32>(vector.x)), y(static_cast<Uint32>(vector.y)), z(static_cast<Uint32>(vector.z))
	{
		/// No Code
	}

	constexpr Uvector3::operator Vector3()const
	{
		return Vector3(static_cast<Float>(x), static_cast<Float>(y), static_cast<Float>(z));
	}

	constexpr Uvector3::operator Ivector3()const
	{
		return Ivector3(static_cast<Int32>(x), static_cast<Int32>(y), static_cast<Int32>(z));
	}

	constexpr Bool Uvector3::operator==(const Uvector3& vector)const
	{
		return x == vector.x && y == vector.y && z == vector.z;
	}

	constexpr Bool Uvector3::operator!=(const Uvector3& vector)const
	{
		return !(*this == vector);
	}

	constexpr Uvector3& Uvector3::operator+=(const Uvector3& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator-=(const Uvector3& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator*=(const Uvector3& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator/=(const Uvector3& vector)
	{
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator%=(const Uvector3& vector)
	{
		x %= vector.x;
		y %= vector.y;
		z %= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator&=(const Uvector3& vector)
	{
		x &= vector.x;
		y &= vector.y;
		z &= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator|=(const Uvector3& vector)
	{
		x |= vector.x;
		y |= vector.y;
		z |= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator^=(const Uvector3& vector)
	{
		x ^= vector.x;
		y ^= vector.y;
		z ^= vector.z;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator*=(Uint32 scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator/=(Uint32 scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator%=(Uint32 scalar)
	{
		x %= scalar;
		y %= scalar;
		z %= scalar;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator<<=(Int32 shift)
	{
		x <<= shift;
		y <<= shift;
		z <<= shift;
		return *this;
	}

	constexpr Uvector3& Uvector3::operator>>=(Int32 shift)
	{
		x >>= shift;
		y >>= shift;
		z >>= shift;
		return *this;
	}

	constexpr Uvector3 Uvector3::operator+()const
	{
		return *this;
	}

	constexpr Uvector3 Uvector3::operator~()const
	{
		return Uvector3(~x, ~y, ~z);
	}

	constexpr Uvector3 Uvector3::operator+(const Uvector3& vector)const
	{
		return Uvector3(x + vector.x, y + vector.y, z + vector.z);
	}

	constexpr Uvector3 Uvector3::operator-(const Uvector3& vector)const
	{
		return Uvector3(x - vector.x, y - vector.y, z - vector.z);
	}

	constexpr Uvector3 Uvector3::operator*(const Uvector3& vector)const
	{
		return Uvector3(x * vector.x, y * vector.y, z * vector.z);
	}

	constexpr Uvector3 Uvector3::operator/(const Uvector3& vector)const
	{
		return Uvector3(x / vector.x, y / vector.y, z / vector.z);
	}

	constexpr Uvector3 Uvector3::operator%(const Uvector3& vector)const
	{
		return Uvector3(x % vector.x, y % vector.y, z % vector.z);
	}

	constexpr Uvector3 Uvector3::operator&(const Uvector3& vector)const
	{
		return Uvector3(x & vector.x, y & vector.y, z & vector.z);
	}

	constexpr Uvector3 Uvector3::operator|(const Uvector3& vector)const
	{
		return Uvector3(x | vector.x, y | vector.y, z | vector.z);
	}

	constexpr Uvector3 Uvector3::operator^(const Uvector3& vector)const
	{
		return Uvector3(x ^ vector.x, y ^ vector.y, z ^ vector.z);
	}

	constexpr Uvector3 Uvector3::operator*(Uint32 scalar)const
	{
		return Uvector3(x * scalar, y * scalar, z * scalar);
	}

	constexpr Uvector3 Uvector3::operator/(Uint32 scalar)const
	{
		return Uvector3(x / scalar, y / scalar, z / scalar);
	}

	constexpr Uvector3 Uvector3::operator%(Uint32 scalar)const
	{
		return Uvector3(x % scalar, y % scalar, z % scalar);
	}

	constexpr Uvector3 Uvector3::operator<<(Int32 shift)const
	{
		return Uvector3(x << shift, y << shift, z << shift);
	}

	constexpr Uvector3 Uvector3::operator>>(Int32 shift)const
	{
		return Uvector3(x >> shift, y >> shift, z >> shift);
	}

	constexpr Uvector3 operator*(Uint32 scalar, const Uvector3& vector)
	{
		return Uvector3(scalar * vector.x, scalar * vector.y, scalar * vector.z);
	}

	constexpr Uint64 Uvector3::LengthSq()const
	{
		return static_cast<Uint64>(x) * x + static_cast<Uint64>(y) * y + static_cast<Uint64>(z) * z;
	}

	constexpr Uint64 Uvector3::Dot(const Uvector3& vector)const
	{
		return static_cast<Uint64>(x) * vector.x + static_cast<Uint64>(y) * vector.y + static_cast<Uint64>(z) * vector.z;
	}

	inline constexpr Uvector3 Uvector3::Zero = { 0, 0, 0 };
	inline constexpr Uvector3 Uvector3::One = { 1, 1, 1 };
	inline constexpr Uvector3 Uvector3::UnitX = { 1, 0, 0 };
	inline constexpr Uvector3 Uvector3::UnitY = { 0, 1, 0 };
	inline constexpr Uvector3 Uvector3::UnitZ = { 0, 0, 1 };
}
