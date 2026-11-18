#pragma once

namespace SeedCore
{
	struct Matrix;
	struct Quaternion;

	struct Vector3
	{
		union
		{
			struct
			{
				Float x;
				Float y;
				Float z;
			};
			struct
			{
				Vector2 xy;
			};
			struct
			{
				Uint32 : 32;
				Vector2 yz;
			};
		};

		constexpr Vector3();
		constexpr Vector3(Float x, Float y, Float z);

		constexpr Bool operator==(const Vector3& vector)const;
		constexpr Bool operator!=(const Vector3& vector)const;

		constexpr Vector3& operator+=(const Vector3& vector);
		constexpr Vector3& operator-=(const Vector3& vector);
		constexpr Vector3& operator*=(const Vector3& vector);
		constexpr Vector3& operator/=(const Vector3& vector);

		constexpr Vector3& operator*=(Float scalar);
		constexpr Vector3& operator/=(Float scalar);

		constexpr Vector3 operator+()const;
		constexpr Vector3 operator-()const;

		constexpr Vector3 operator+(const Vector3& vector)const;
		constexpr Vector3 operator-(const Vector3& vector)const;
		constexpr Vector3 operator*(const Vector3& vector)const;
		constexpr Vector3 operator/(const Vector3& vector)const;

		constexpr Vector3 operator*(Float scalar)const;
		constexpr Vector3 operator/(Float scalar)const;

		friend constexpr Vector3 operator*(Float scalar, const Vector3& vector);

		NODISCARD constexpr Float Length()const;
		NODISCARD constexpr Float LengthSq()const;

		NODISCARD static constexpr Float Distance(const Vector3& vector1, const Vector3& vector2);
		NODISCARD static constexpr Float DistanceSq(const Vector3& vector1, const Vector3& vector2);

		NODISCARD constexpr Float Dot(const Vector3& vector)const;
		NODISCARD constexpr Vector3 Cross(const Vector3& vector)const;

		constexpr void Normalize();

		NODISCARD static constexpr Vector3 Min(const Vector3& vector1, const Vector3& vector2);
		NODISCARD static constexpr Vector3 Max(const Vector3& vector1, const Vector3& vector2);

		NODISCARD static constexpr Vector3 Lerp(const Vector3& vector1, const Vector3& vector2, Float t);

		NODISCARD static constexpr Vector3 Transform(const Vector3& vector, const Quaternion& rotation);

		NODISCARD static constexpr Vector3 TransformPosition(const Vector3& position, const Matrix& matrix);
		NODISCARD static constexpr Vector3 TransformCoordinate(const Vector3& coordinate, const Matrix& matrix);
		NODISCARD static constexpr Vector3 TransformDirection(const Vector3& direction, const Matrix& matrix);
		NODISCARD static constexpr Vector3 TransformNormal(const Vector3& normal, const Matrix& matrix);
		
		static const Vector3 Zero;
		static const Vector3 One;
		static const Vector3 UnitX;
		static const Vector3 UnitY;
		static const Vector3 UnitZ;

		static const Vector3 Up;
		static const Vector3 Down;
		static const Vector3 Right;
		static const Vector3 Left;
		static const Vector3 Forward;
		static const Vector3 Backward;
	};

	constexpr Vector3::Vector3() :x(0.0f), y(0.0f), z(0.0f)
	{
		/// No Code
	}

	constexpr Vector3::Vector3(Float x, Float y, Float z) : x(x), y(y), z(z)
	{
		/// No Code
	}

	constexpr Bool Vector3::operator==(const Vector3& vector)const
	{
		return x == vector.x && y == vector.y && z == vector.z;
	}

	constexpr Bool Vector3::operator!=(const Vector3& vector)const
	{
		return !(*this == vector);
	}

	constexpr Vector3& Vector3::operator+=(const Vector3& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		return *this;
	}

	constexpr Vector3& Vector3::operator-=(const Vector3& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		return *this;
	}

	constexpr Vector3& Vector3::operator*=(const Vector3& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		return *this;
	}

	constexpr Vector3& Vector3::operator/=(const Vector3& vector)
	{
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		return *this;
	}

	constexpr Vector3& Vector3::operator*=(Float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	constexpr Vector3& Vector3::operator/=(Float scalar)
	{
		Float inverse = 1.0f / scalar;
		x *= inverse;
		y *= inverse;
		z *= inverse;
		return *this;
	}

	constexpr Vector3 Vector3::operator+()const
	{
		return *this;
	}

	constexpr Vector3 Vector3::operator-()const
	{
		return Vector3(-x, -y, -z);
	}

	constexpr Vector3 Vector3::operator+(const Vector3& vector)const
	{
		return Vector3(x + vector.x, y + vector.y, z + vector.z);
	}

	constexpr Vector3 Vector3::operator-(const Vector3& vector)const
	{
		return Vector3(x - vector.x, y - vector.y, z - vector.z);
	}

	constexpr Vector3 Vector3::operator*(const Vector3& vector)const
	{
		return Vector3(x * vector.x, y * vector.y, z * vector.z);
	}

	constexpr Vector3 Vector3::operator/(const Vector3& vector)const
	{
		return Vector3(x / vector.x, y / vector.y, z / vector.z);
	}

	constexpr Vector3 Vector3::operator*(Float scalar)const
	{
		return Vector3(x * scalar, y * scalar, z * scalar);
	}

	constexpr Vector3 Vector3::operator/(Float scalar)const
	{
		Float inverse = 1.0f / scalar;
		return Vector3(x * inverse, y * inverse, z * inverse);
	}

	constexpr Vector3 operator*(Float scalar, const Vector3& vector)
	{
		return Vector3(scalar * vector.x, scalar * vector.y, scalar * vector.z);
	}

	constexpr Float Vector3::Length()const
	{
		Float squared = x * x + y * y + z * z;
		if consteval
		{
			return SeedCore::Sqrt(squared);
		}
		else
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(squared)));
		}
	}

	constexpr Float Vector3::LengthSq()const
	{
		return x * x + y * y + z * z;
	}

	constexpr Float Vector3::Distance(const Vector3& vector1, const Vector3& vector2)
	{
		return (vector2 - vector1).Length();
	}

	constexpr Float Vector3::DistanceSq(const Vector3& vector1, const Vector3& vector2)
	{
		return (vector2 - vector1).LengthSq();
	}

	constexpr Float Vector3::Dot(const Vector3& vector)const
	{
		return x * vector.x + y * vector.y + z * vector.z;
	}

	constexpr Vector3 Vector3::Cross(const Vector3& vector)const
	{
		return Vector3(y * vector.z - z * vector.y, z * vector.x - x * vector.z, x * vector.y - y * vector.x);
	}

	constexpr void Vector3::Normalize()
	{
		Float length = Length();
		if (length > 0.0f)
		{
			Float inverse = 1.0f / length;
			x *= inverse;
			y *= inverse;
			z *= inverse;
		}
	}

	constexpr Vector3 Vector3::Min(const Vector3& vector1, const Vector3& vector2)
	{
		return Vector3(vector1.x < vector2.x ? vector1.x : vector2.x, vector1.y < vector2.y ? vector1.y : vector2.y, vector1.z < vector2.z ? vector1.z : vector2.z);
	}

	constexpr Vector3 Vector3::Max(const Vector3& vector1, const Vector3& vector2)
	{
		return Vector3(vector1.x > vector2.x ? vector1.x : vector2.x, vector1.y > vector2.y ? vector1.y : vector2.y, vector1.z > vector2.z ? vector1.z : vector2.z);
	}

	constexpr Vector3 Vector3::Lerp(const Vector3& vector1, const Vector3& vector2, Float t)
	{
		return Vector3(vector1.x + (vector2.x - vector1.x) * t, vector1.y + (vector2.y - vector1.y) * t, vector1.z + (vector2.z - vector1.z) * t);
	}

	inline constexpr Vector3 Vector3::Zero = { 0.0f, 0.0f, 0.0f };
	inline constexpr Vector3 Vector3::One = { 1.0f, 1.0f, 1.0f };
	inline constexpr Vector3 Vector3::UnitX = { 1.0f, 0.0f, 0.0f };
	inline constexpr Vector3 Vector3::UnitY = { 0.0f, 1.0f, 0.0f };
	inline constexpr Vector3 Vector3::UnitZ = { 0.0f, 0.0f, 1.0f };

	inline constexpr Vector3 Vector3::Up = { 0.0f, 1.0f, 0.0f };
	inline constexpr Vector3 Vector3::Down = { 0.0f, -1.0f, 0.0f };
	inline constexpr Vector3 Vector3::Right = { 1.0f, 0.0f, 0.0f };
	inline constexpr Vector3 Vector3::Left = { -1.0f, 0.0f, 0.0f };
	inline constexpr Vector3 Vector3::Forward = { 0.0f, 0.0f, 1.0f };
	inline constexpr Vector3 Vector3::Backward = { 0.0f, 0.0f, -1.0f };
}