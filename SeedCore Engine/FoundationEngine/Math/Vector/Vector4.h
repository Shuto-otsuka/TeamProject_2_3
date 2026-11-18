#pragma once

namespace SeedCore
{
	struct Color;
	struct Matrix;

	struct Vector4
	{
		union
		{
			struct
			{
				Float x;
				Float y;
				Float z;
				Float w;
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
			struct
			{
				Uint32 : 32;
				Uint32 : 32;
				Vector2 zw;
			};
			struct
			{
				Vector3 xyz;
			};
			struct
			{
				Uint32 : 32;
				Vector3 yzw;
			};
		};

		constexpr Vector4();
		constexpr Vector4(Float x, Float y, Float z, Float w);
		constexpr explicit Vector4(const Vector3& xyz, Float w);
		constexpr explicit Vector4(const Color& color);

		constexpr Bool operator==(const Vector4& vector)const;
		constexpr Bool operator!=(const Vector4& vector)const;

		constexpr Vector4& operator+=(const Vector4& vector);
		constexpr Vector4& operator-=(const Vector4& vector);
		constexpr Vector4& operator*=(const Vector4& vector);
		constexpr Vector4& operator/=(const Vector4& vector);

		constexpr Vector4& operator*=(Float scalar);
		constexpr Vector4& operator/=(Float scalar);

		constexpr Vector4 operator+()const;
		constexpr Vector4 operator-()const;

		constexpr Vector4 operator+(const Vector4& vector)const;
		constexpr Vector4 operator-(const Vector4& vector)const;
		constexpr Vector4 operator*(const Vector4& vector)const;
		constexpr Vector4 operator/(const Vector4& vector)const;

		constexpr Vector4 operator*(Float scalar)const;
		constexpr Vector4 operator/(Float scalar)const;

		friend constexpr Vector4 operator*(Float scalar, const Vector4& vector);

		NODISCARD constexpr Float Length()const;
		NODISCARD constexpr Float LengthSq()const;

		NODISCARD static constexpr Float Distance(const Vector4& vector1, const Vector4& vector2);
		NODISCARD static constexpr Float DistanceSq(const Vector4& vector1, const Vector4& vector2);

		NODISCARD constexpr Float Dot(const Vector4& vector)const;

		constexpr void Normalize();

		NODISCARD static constexpr Vector4 Min(const Vector4& vector1, const Vector4& vector2);
		NODISCARD static constexpr Vector4 Max(const Vector4& vector1, const Vector4& vector2);

		NODISCARD static constexpr Vector4 Lerp(const Vector4& vector1, const Vector4& vector2, Float t);

		NODISCARD static constexpr Vector4 Transform(const Vector4& vector, const Matrix& matrix);

		static const Vector4 Zero;
		static const Vector4 One;
		static const Vector4 UnitX;
		static const Vector4 UnitY;
		static const Vector4 UnitZ;
		static const Vector4 UnitW;
	};

	constexpr Vector4::Vector4() :x(0.0f), y(0.0f), z(0.0f), w(0.0f)
	{
		/// No Code
	}

	constexpr Vector4::Vector4(Float x, Float y, Float z, Float w) : x(x), y(y), z(z), w(w)
	{
		/// No Code
	}

	constexpr Vector4::Vector4(const Vector3& xyz, Float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
	{
		/// No Code
	}

	constexpr Bool Vector4::operator==(const Vector4& vector)const
	{
		return x == vector.x && y == vector.y && z == vector.z && w == vector.w;
	}

	constexpr Bool Vector4::operator!=(const Vector4& vector)const
	{
		return !(*this == vector);
	}

	constexpr Vector4& Vector4::operator+=(const Vector4& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		w += vector.w;
		return *this;
	}

	constexpr Vector4& Vector4::operator-=(const Vector4& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		w -= vector.w;
		return *this;
	}

	constexpr Vector4& Vector4::operator*=(const Vector4& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		w *= vector.w;
		return *this;
	}

	constexpr Vector4& Vector4::operator/=(const Vector4& vector)
	{
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		w /= vector.w;
		return *this;
	}

	constexpr Vector4& Vector4::operator*=(Float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	constexpr Vector4& Vector4::operator/=(Float scalar)
	{
		Float inverse = 1.0f / scalar;
		x *= inverse;
		y *= inverse;
		z *= inverse;
		w *= inverse;
		return *this;
	}

	constexpr Vector4 Vector4::operator+()const
	{
		return *this;
	}

	constexpr Vector4 Vector4::operator-()const
	{
		return Vector4(-x, -y, -z, -w);
	}

	constexpr Vector4 Vector4::operator+(const Vector4& vector)const
	{
		return Vector4(x + vector.x, y + vector.y, z + vector.z, w + vector.w);
	}

	constexpr Vector4 Vector4::operator-(const Vector4& vector)const
	{
		return Vector4(x - vector.x, y - vector.y, z - vector.z, w - vector.w);
	}

	constexpr Vector4 Vector4::operator*(const Vector4& vector)const
	{
		return Vector4(x * vector.x, y * vector.y, z * vector.z, w * vector.w);
	}

	constexpr Vector4 Vector4::operator/(const Vector4& vector)const
	{
		return Vector4(x / vector.x, y / vector.y, z / vector.z, w / vector.w);
	}

	constexpr Vector4 Vector4::operator*(Float scalar)const
	{
		return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	constexpr Vector4 Vector4::operator/(Float scalar)const
	{
		Float inverse = 1.0f / scalar;
		return Vector4(x * inverse, y * inverse, z * inverse, w * inverse);
	}

	constexpr Vector4 operator*(Float scalar, const Vector4& vector)
	{
		return Vector4(scalar * vector.x, scalar * vector.y, scalar * vector.z, scalar * vector.w);
	}

	constexpr Float Vector4::Length()const
	{
		Float squared = x * x + y * y + z * z + w * w;
		if consteval
		{
			return SeedCore::Sqrt(squared);
		}
		else
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(squared)));
		}
	}

	constexpr Float Vector4::LengthSq()const
	{
		return x * x + y * y + z * z + w * w;
	}

	constexpr Float Vector4::Distance(const Vector4& vector1, const Vector4& vector2)
	{
		return (vector2 - vector1).Length();
	}

	constexpr Float Vector4::DistanceSq(const Vector4& vector1, const Vector4& vector2)
	{
		return (vector2 - vector1).LengthSq();
	}

	constexpr Float Vector4::Dot(const Vector4& vector)const
	{
		return x * vector.x + y * vector.y + z * vector.z + w * vector.w;
	}

	constexpr void Vector4::Normalize()
	{
		Float length = Length();
		if (length > 0.0f)
		{
			Float inverse = 1.0f / length;
			x *= inverse;
			y *= inverse;
			z *= inverse;
			w *= inverse;
		}
	}

	constexpr Vector4 Vector4::Min(const Vector4& vector1, const Vector4& vector2)
	{
		return Vector4(vector1.x < vector2.x ? vector1.x : vector2.x, vector1.y < vector2.y ? vector1.y : vector2.y, vector1.z < vector2.z ? vector1.z : vector2.z, vector1.w < vector2.w ? vector1.w : vector2.w);
	}

	constexpr Vector4 Vector4::Max(const Vector4& vector1, const Vector4& vector2)
	{
		return Vector4(vector1.x > vector2.x ? vector1.x : vector2.x, vector1.y > vector2.y ? vector1.y : vector2.y, vector1.z > vector2.z ? vector1.z : vector2.z, vector1.w > vector2.w ? vector1.w : vector2.w);
	}

	constexpr Vector4 Vector4::Lerp(const Vector4& vector1, const Vector4& vector2, Float t)
	{
		return Vector4(vector1.x + (vector2.x - vector1.x) * t, vector1.y + (vector2.y - vector1.y) * t, vector1.z + (vector2.z - vector1.z) * t, vector1.w + (vector2.w - vector1.w) * t);
	}

	inline constexpr Vector4 Vector4::Zero = { 0.0f, 0.0f, 0.0f, 0.0f };
	inline constexpr Vector4 Vector4::One = { 1.0f, 1.0f, 1.0f, 1.0f };
	inline constexpr Vector4 Vector4::UnitX = { 1.0f, 0.0f, 0.0f, 0.0f };
	inline constexpr Vector4 Vector4::UnitY = { 0.0f, 1.0f, 0.0f, 0.0f };
	inline constexpr Vector4 Vector4::UnitZ = { 0.0f, 0.0f, 1.0f, 0.0f };
	inline constexpr Vector4 Vector4::UnitW = { 0.0f, 0.0f, 0.0f, 1.0f };
}