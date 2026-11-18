#pragma once

namespace SeedCore
{
	struct Vector2
	{
		Float x;
		Float y;

		constexpr Vector2();
		constexpr explicit Vector2(Float value);
		constexpr Vector2(Float x, Float y);
		constexpr explicit Vector2(const Float* array);

		constexpr Bool operator==(const Vector2& vector)const;
		constexpr Bool operator!=(const Vector2& vector)const;

		constexpr Vector2& operator+=(const Vector2& vector);
		constexpr Vector2& operator-=(const Vector2& vector);
		constexpr Vector2& operator*=(const Vector2& vector);
		constexpr Vector2& operator/=(const Vector2& vector);

		constexpr Vector2& operator*=(Float value);
		constexpr Vector2& operator/=(Float value);

		constexpr Vector2 operator+()const;
		constexpr Vector2 operator-()const;

		constexpr Vector2 operator+(const Vector2& vector)const;
		constexpr Vector2 operator-(const Vector2& vector)const;
		constexpr Vector2 operator*(const Vector2& vector)const;
		constexpr Vector2 operator/(const Vector2& vector)const;

		constexpr Vector2 operator*(Float scalar)const;
		constexpr Vector2 operator/(Float scalar)const;

		friend constexpr Vector2 operator*(Float scalar, const Vector2& vector);

		NODISCARD constexpr Float Length()const;
		NODISCARD constexpr Float LengthSq()const;

		NODISCARD static constexpr Float Distance(const Vector2& vector1, const Vector2& vector2);
		NODISCARD static constexpr Float DistanceSq(const Vector2& vector1, const Vector2& vector2);

		NODISCARD constexpr Float Dot(const Vector2& vector)const;
		NODISCARD constexpr Float Cross(const Vector2& vector)const;

		constexpr void Normalize();

		NODISCARD static constexpr Vector2 Min(const Vector2& vector1, const Vector2& vector2);
		NODISCARD static constexpr Vector2 Max(const Vector2& vector1, const Vector2& vector2);

		NODISCARD static constexpr Vector2 Lerp(const Vector2& vector1, Vector2& vector2, Float t);

		static const Vector2 Zero;
		static const Vector2 One;
		static const Vector2 UnitX;
		static const Vector2 UnitY;
	};

	constexpr Vector2::Vector2() :x(0.0f), y(0.0f)
	{
		/// No Code
	}

	constexpr Vector2::Vector2(Float x, Float y) : x(x), y(y)
	{
		/// No Code
	}

	constexpr Bool Vector2::operator==(const Vector2& vector)const
	{
		return x == vector.x && y == vector.y;
	}

	constexpr Bool Vector2::operator!=(const Vector2& vector)const
	{
		return !(*this == vector);
	}

	constexpr Vector2& Vector2::operator+=(const Vector2& vector)
	{
		x += vector.x;
		y += vector.y;
		return *this;
	}

	constexpr Vector2& Vector2::operator-=(const Vector2& vector)
	{
		x -= vector.x;
		y -= vector.y;
		return *this;
	}

	constexpr Vector2& Vector2::operator*=(const Vector2& vector)
	{
		x *= vector.x;
		y *= vector.y;
		return *this;
	}

	constexpr Vector2& Vector2::operator/=(const Vector2& vector)
	{
		x /= vector.x;
		y /= vector.y;
		return *this;
	}

	constexpr Vector2& Vector2::operator*=(Float scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	constexpr Vector2& Vector2::operator/=(Float scalar)
	{
		Float inverse = 1.0f / scalar;
		y *= inverse;
		x *= inverse;
		return *this;
	}

	constexpr Vector2 Vector2::operator+()const
	{
		return *this;
	}

	constexpr Vector2 Vector2::operator-()const
	{
		return Vector2(-x, -y);
	}

	constexpr Vector2 Vector2::operator+(const Vector2& vector)const
	{
		return Vector2(x + vector.x, y + vector.y);
	}

	constexpr Vector2 Vector2::operator-(const Vector2& vector)const
	{
		return Vector2(x - vector.x, y - vector.y);
	}

	constexpr Vector2 Vector2::operator*(const Vector2& vector)const
	{
		return Vector2(x * vector.x, y * vector.y);
	}

	constexpr Vector2 Vector2::operator/(const Vector2& vector)const
	{
		return Vector2(x / vector.x, y / vector.y);
	}

	constexpr Vector2 Vector2::operator*(Float scalar)const
	{
		return Vector2(x * scalar, y * scalar);
	}

	constexpr Vector2 Vector2::operator/(Float scalar)const
	{
		Float inverse = 1.0f / scalar;
		return Vector2(x * inverse, y * inverse);
	}

	constexpr Vector2 operator*(Float scalar, const Vector2& vector)
	{
		return Vector2(scalar * vector.x, scalar * vector.y);
	}

	constexpr Float Vector2::Length()const
	{
		Float squared = x * x + y * y;
		if consteval
		{
			return SeedCore::Sqrt(squared);
		}
		else
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(squared)));
		}
	}

	constexpr Float Vector2::LengthSq()const
	{
		return x * x + y * y;
	}

	constexpr Float Vector2::Distance(const Vector2& vector1, const Vector2& vector2)
	{
		return (vector2 - vector1).Length();
	}

	constexpr Float Vector2::DistanceSq(const Vector2& vector1, const Vector2& vector2)
	{
		return (vector2 - vector1).LengthSq();
	}

	constexpr Float Vector2::Dot(const Vector2& vector)const
	{
		return x * vector.x + y * vector.y;
	}

	constexpr Float Vector2::Cross(const Vector2& vector)const
	{
		return x * vector.y - y * vector.x;
	}

	constexpr void Vector2::Normalize()
	{
		Float length = Length();
		if (length > 0.0f)
		{
			Float inverse = 1.0f / length;
			x *= inverse;
			y *= inverse;
		}
	}

	constexpr Vector2 Vector2::Min(const Vector2& vector1, const Vector2& vector2)
	{
		return Vector2(vector1.x < vector2.x ? vector1.x : vector2.x, vector1.y < vector2.y ? vector1.y : vector2.y);
	}

	constexpr Vector2 Vector2::Max(const Vector2& vector1, const Vector2& vector2)
	{
		return Vector2(vector1.x > vector2.x ? vector1.x : vector2.x, vector1.y > vector2.y ? vector1.y : vector2.y);
	}

	constexpr Vector2 Vector2::Lerp(const Vector2& vector1, Vector2& vector2, Float t)
	{
		return Vector2(vector1.x + (vector2.x - vector1.x) * t, vector1.y + (vector2.y - vector1.y) * t);
	}

	inline constexpr Vector2 Vector2::Zero = { 0.0f, 0.0f };
	inline constexpr Vector2 Vector2::One = { 1.0f, 1.0f };
	inline constexpr Vector2 Vector2::UnitX = { 1.0f, 0.0f };
	inline constexpr Vector2 Vector2::UnitY = { 0.0f, 1.0f };
}