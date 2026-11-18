#pragma once

namespace SeedCore
{
	struct Ivector3
	{
		union
		{
			struct
			{
				Int32 x;
				Int32 y;
				Int32 z;
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
		};

		constexpr Ivector3();
		constexpr Ivector3(Int32 x, Int32 y, Int32 z);
		constexpr explicit Ivector3(const Vector3& vector);

		constexpr explicit operator Vector3()const;

		constexpr Bool operator==(const Ivector3& vector)const;
		constexpr Bool operator!=(const Ivector3& vector)const;

		constexpr Ivector3& operator+=(const Ivector3& vector);
		constexpr Ivector3& operator-=(const Ivector3& vector);
		constexpr Ivector3& operator*=(const Ivector3& vector);
		constexpr Ivector3& operator/=(const Ivector3& vector);
		constexpr Ivector3& operator%=(const Ivector3& vector);
		constexpr Ivector3& operator&=(const Ivector3& vector);
		constexpr Ivector3& operator|=(const Ivector3& vector);
		constexpr Ivector3& operator^=(const Ivector3& vector);

		constexpr Ivector3& operator*=(Int32 scalar);
		constexpr Ivector3& operator/=(Int32 scalar);
		constexpr Ivector3& operator%=(Int32 scalar);
		constexpr Ivector3& operator<<=(Int32 shift);
		constexpr Ivector3& operator>>=(Int32 shift);

		constexpr Ivector3 operator+()const;
		constexpr Ivector3 operator-()const;
		constexpr Ivector3 operator~()const;

		constexpr Ivector3 operator+(const Ivector3& vector)const;
		constexpr Ivector3 operator-(const Ivector3& vector)const;
		constexpr Ivector3 operator*(const Ivector3& vector)const;
		constexpr Ivector3 operator/(const Ivector3& vector)const;
		constexpr Ivector3 operator%(const Ivector3& vector)const;
		constexpr Ivector3 operator&(const Ivector3& vector)const;
		constexpr Ivector3 operator|(const Ivector3& vector)const;
		constexpr Ivector3 operator^(const Ivector3& vector)const;

		constexpr Ivector3 operator*(Int32 scalar)const;
		constexpr Ivector3 operator/(Int32 scalar)const;
		constexpr Ivector3 operator%(Int32 scalar)const;
		constexpr Ivector3 operator<<(Int32 shift)const;
		constexpr Ivector3 operator>>(Int32 shift)const;

		friend constexpr Ivector3 operator*(Int32 scalar, const Ivector3& vector);

		NODISCARD constexpr Int64 LengthSq()const;

		NODISCARD constexpr Int64 Dot(const Ivector3& vector)const;

		static const Ivector3 Zero;
		static const Ivector3 One;
		static const Ivector3 UnitX;
		static const Ivector3 UnitY;
		static const Ivector3 UnitZ;
	};

	constexpr Ivector3::Ivector3() :x(0), y(0), z(0)
	{
		/// No Code
	}

	constexpr Ivector3::Ivector3(Int32 x, Int32 y, Int32 z) :x(x), y(y), z(z)
	{
		/// No Code
	}

	constexpr Ivector3::Ivector3(const Vector3& vector) :x(static_cast<Int32>(vector.x)), y(static_cast<Int32>(vector.y)), z(static_cast<Int32>(vector.z))
	{
		/// No Code
	}

	constexpr Ivector3::operator Vector3()const
	{
		return Vector3(static_cast<Float>(x), static_cast<Float>(y), static_cast<Float>(z));
	}

	constexpr Bool Ivector3::operator==(const Ivector3& vector)const
	{
		return x == vector.x && y == vector.y && z == vector.z;
	}

	constexpr Bool Ivector3::operator!=(const Ivector3& vector)const
	{
		return !(*this == vector);
	}

	constexpr Ivector3& Ivector3::operator+=(const Ivector3& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator-=(const Ivector3& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator*=(const Ivector3& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator/=(const Ivector3& vector)
	{
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator%=(const Ivector3& vector)
	{
		x %= vector.x;
		y %= vector.y;
		z %= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator&=(const Ivector3& vector)
	{
		x &= vector.x;
		y &= vector.y;
		z &= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator|=(const Ivector3& vector)
	{
		x |= vector.x;
		y |= vector.y;
		z |= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator^=(const Ivector3& vector)
	{
		x ^= vector.x;
		y ^= vector.y;
		z ^= vector.z;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator*=(Int32 scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator/=(Int32 scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator%=(Int32 scalar)
	{
		x %= scalar;
		y %= scalar;
		z %= scalar;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator<<=(Int32 shift)
	{
		x <<= shift;
		y <<= shift;
		z <<= shift;
		return *this;
	}

	constexpr Ivector3& Ivector3::operator>>=(Int32 shift)
	{
		x >>= shift;
		y >>= shift;
		z >>= shift;
		return *this;
	}

	constexpr Ivector3 Ivector3::operator+()const
	{
		return *this;
	}

	constexpr Ivector3 Ivector3::operator-()const
	{
		return Ivector3(-x, -y, -z);
	}

	constexpr Ivector3 Ivector3::operator~()const
	{
		return Ivector3(~x, ~y, ~z);
	}

	constexpr Ivector3 Ivector3::operator+(const Ivector3& vector)const
	{
		return Ivector3(x + vector.x, y + vector.y, z + vector.z);
	}

	constexpr Ivector3 Ivector3::operator-(const Ivector3& vector)const
	{
		return Ivector3(x - vector.x, y - vector.y, z - vector.z);
	}

	constexpr Ivector3 Ivector3::operator*(const Ivector3& vector)const
	{
		return Ivector3(x * vector.x, y * vector.y, z * vector.z);
	}

	constexpr Ivector3 Ivector3::operator/(const Ivector3& vector)const
	{
		return Ivector3(x / vector.x, y / vector.y, z / vector.z);
	}

	constexpr Ivector3 Ivector3::operator%(const Ivector3& vector)const
	{
		return Ivector3(x % vector.x, y % vector.y, z % vector.z);
	}

	constexpr Ivector3 Ivector3::operator&(const Ivector3& vector)const
	{
		return Ivector3(x & vector.x, y & vector.y, z & vector.z);
	}

	constexpr Ivector3 Ivector3::operator|(const Ivector3& vector)const
	{
		return Ivector3(x | vector.x, y | vector.y, z | vector.z);
	}

	constexpr Ivector3 Ivector3::operator^(const Ivector3& vector)const
	{
		return Ivector3(x ^ vector.x, y ^ vector.y, z ^ vector.z);
	}

	constexpr Ivector3 Ivector3::operator*(Int32 scalar)const
	{
		return Ivector3(x * scalar, y * scalar, z * scalar);
	}

	constexpr Ivector3 Ivector3::operator/(Int32 scalar)const
	{
		return Ivector3(x / scalar, y / scalar, z / scalar);
	}

	constexpr Ivector3 Ivector3::operator%(Int32 scalar)const
	{
		return Ivector3(x % scalar, y % scalar, z % scalar);
	}

	constexpr Ivector3 Ivector3::operator<<(Int32 shift)const
	{
		return Ivector3(x << shift, y << shift, z << shift);
	}

	constexpr Ivector3 Ivector3::operator>>(Int32 shift)const
	{
		return Ivector3(x >> shift, y >> shift, z >> shift);
	}

	constexpr Ivector3 operator*(Int32 scalar, const Ivector3& vector)
	{
		return Ivector3(scalar * vector.x, scalar * vector.y, scalar * vector.z);
	}

	constexpr Int64 Ivector3::LengthSq()const
	{
		return static_cast<Int64>(x) * x + static_cast<Int64>(y) * y + static_cast<Int64>(z) * z;
	}

	constexpr Int64 Ivector3::Dot(const Ivector3& vector)const
	{
		return static_cast<Int64>(x) * vector.x + static_cast<Int64>(y) * vector.y + static_cast<Int64>(z) * vector.z;
	}

	inline constexpr Ivector3 Ivector3::Zero = { 0, 0, 0 };
	inline constexpr Ivector3 Ivector3::One = { 1, 1, 1 };
	inline constexpr Ivector3 Ivector3::UnitX = { 1, 0, 0 };
	inline constexpr Ivector3 Ivector3::UnitY = { 0, 1, 0 };
	inline constexpr Ivector3 Ivector3::UnitZ = { 0, 0, 1 };
}
