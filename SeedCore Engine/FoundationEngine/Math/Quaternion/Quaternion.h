#pragma once

namespace SeedCore
{
	struct Matrix;

	struct Quaternion
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
				Vector3 xyz;
			};
		};

		constexpr Quaternion();
		constexpr Quaternion(Float x, Float y, Float z, Float w);

		constexpr Bool operator==(const Quaternion& quaternion)const;
		constexpr Bool operator!=(const Quaternion& quaternion)const;

		constexpr Quaternion& operator+=(const Quaternion& quaternion);
		constexpr Quaternion& operator-=(const Quaternion& quaternion);
		constexpr Quaternion& operator*=(const Quaternion& quaternion);
		constexpr Quaternion& operator/=(const Quaternion& quaternion);

		constexpr Quaternion& operator*=(Float scalar);
		constexpr Quaternion& operator/=(Float scalar);

		constexpr Quaternion operator+()const;
		constexpr Quaternion operator-()const;

		constexpr Quaternion operator+(const Quaternion& quaternion)const;
		constexpr Quaternion operator-(const Quaternion& quaternion)const;
		constexpr Quaternion operator*(const Quaternion& quaternion)const;
		constexpr Quaternion operator/(const Quaternion& quaternion)const;

		constexpr Quaternion operator*(Float scalar)const;
		constexpr Quaternion operator/(Float scalar)const;

		friend constexpr Quaternion operator*(Float scalar, const Quaternion& quaternion);

		NODISCARD constexpr Float Length()const;
		NODISCARD constexpr Float LengthSq()const;

		NODISCARD constexpr Float Dot(const Quaternion& quaternion)const;

		constexpr void Normalize();

		NODISCARD constexpr Quaternion Conjugate()const;
		NODISCARD constexpr Quaternion Inverse()const;

		NODISCARD constexpr Vector3 ToEuler()const;
		NODISCARD static constexpr Quaternion FromEuler(const Vector3& angles);

		NODISCARD static constexpr Quaternion CreateFromAxisAngle(const Vector3& axis, Float angle);
		NODISCARD static constexpr Quaternion CreateFromYawPitchRoll(Float yaw, Float pitch, Float roll);
		NODISCARD static constexpr Quaternion CreateFromRotationMatrix(const Matrix& matrix);

		NODISCARD static constexpr Quaternion FromToRotation(const Vector3& from, const Vector3& to);
		NODISCARD static constexpr Quaternion LookRotation(const Vector3& forward, const Vector3& up);

		NODISCARD static constexpr Quaternion Slerp(const Quaternion& quaternion1, const Quaternion& quaternion2, Float t);

		static const Quaternion Identity;
	};

	constexpr Quaternion::Quaternion() :x(0.0f), y(0.0f), z(0.0f), w(1.0f)
	{
		/// No Code
	}

	constexpr Quaternion::Quaternion(Float x, Float y, Float z, Float w) : x(x), y(y), z(z), w(w)
	{
		/// No Code
	}

	constexpr Bool Quaternion::operator==(const Quaternion& quaternion)const
	{
		return x == quaternion.x && y == quaternion.y && z == quaternion.z && w == quaternion.w;
	}

	constexpr Bool Quaternion::operator!=(const Quaternion& quaternion)const
	{
		return !(*this == quaternion);
	}

	constexpr Quaternion& Quaternion::operator+=(const Quaternion& quaternion)
	{
		x += quaternion.x;
		y += quaternion.y;
		z += quaternion.z;
		w += quaternion.w;
		return *this;
	}

	constexpr Quaternion& Quaternion::operator-=(const Quaternion& quaternion)
	{
		x -= quaternion.x;
		y -= quaternion.y;
		z -= quaternion.z;
		w -= quaternion.w;
		return *this;
	}

	constexpr Quaternion& Quaternion::operator*=(const Quaternion& quaternion)
	{
		*this = *this * quaternion;
		return *this;
	}

	constexpr Quaternion& Quaternion::operator/=(const Quaternion& quaternion)
	{
		*this = *this * quaternion.Inverse();
		return *this;
	}

	constexpr Quaternion& Quaternion::operator*=(Float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	constexpr Quaternion& Quaternion::operator/=(Float scalar)
	{
		Float inverse = 1.0f / scalar;
		x *= inverse;
		y *= inverse;
		z *= inverse;
		w *= inverse;
		return *this;
	}

	constexpr Quaternion Quaternion::operator+()const
	{
		return *this;
	}

	constexpr Quaternion Quaternion::operator-()const
	{
		return Quaternion(-x, -y, -z, -w);
	}

	constexpr Quaternion Quaternion::operator+(const Quaternion& quaternion)const
	{
		return Quaternion(x + quaternion.x, y + quaternion.y, z + quaternion.z, w + quaternion.w);
	}

	constexpr Quaternion Quaternion::operator-(const Quaternion& quaternion)const
	{
		return Quaternion(x - quaternion.x, y - quaternion.y, z - quaternion.z, w - quaternion.w);
	}

	constexpr Quaternion Quaternion::operator*(const Quaternion& quaternion)const
	{
		return Quaternion(
			quaternion.w * x + quaternion.x * w + quaternion.y * z - quaternion.z * y,
			quaternion.w * y - quaternion.x * z + quaternion.y * w + quaternion.z * x,
			quaternion.w * z + quaternion.x * y - quaternion.y * x + quaternion.z * w,
			quaternion.w * w - quaternion.x * x - quaternion.y * y - quaternion.z * z
			);
	}

	constexpr Quaternion Quaternion::operator/(const Quaternion& quaternion)const
	{
		return *this * quaternion.Inverse();
	}

	constexpr Quaternion Quaternion::operator*(Float scalar)const
	{
		return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	constexpr Quaternion Quaternion::operator/(Float scalar)const
	{
		Float inverse = 1.0f / scalar;
		return Quaternion(x * inverse, y * inverse, z * inverse, w * inverse);
	}

	constexpr Quaternion operator*(Float scalar, const Quaternion& quaternion)
	{
		return Quaternion(scalar * quaternion.x, scalar * quaternion.y, scalar * quaternion.z, scalar * quaternion.w);
	}

	constexpr Float Quaternion::Length()const
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

	constexpr Float Quaternion::LengthSq()const
	{
		return x * x + y * y + z * z + w * w;
	}

	constexpr Float Quaternion::Dot(const Quaternion& quaternion)const
	{
		return x * quaternion.x + y * quaternion.y + z * quaternion.z + w * quaternion.w;
	}

	constexpr void Quaternion::Normalize()
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

	constexpr Quaternion Quaternion::Conjugate()const
	{
		return Quaternion(-x, -y, -z, w);
	}

	constexpr Quaternion Quaternion::Inverse()const
	{
		Float lengthSq = x * x + y * y + z * z + w * w;
		if (lengthSq <= 1.192092896e-7f)
		{
			return Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
		}
		Float inverse = 1.0f / lengthSq;
		return Quaternion(-x * inverse, -y * inverse, -z * inverse, w * inverse);
	}

	constexpr Vector3 Quaternion::ToEuler()const
	{
		Float xx = x * x;
		Float yy = y * y;
		Float zz = z * z;

		Float m31 = 2.0f * x * z + 2.0f * y * w;
		Float m32 = 2.0f * y * z - 2.0f * x * w;
		Float m33 = 1.0f - 2.0f * xx - 2.0f * yy;

		Float cosineY = 0.0f;
		if consteval
		{
			cosineY = SeedCore::Sqrt(m33 * m33 + m31 * m31);
		}
		else
		{
			cosineY = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(m33 * m33 + m31 * m31)));
		}
		Float pitch = SeedCore::Atan2(-m32, cosineY);

		if (cosineY > 16.0f * 1.192092896e-7f)
		{
			Float m12 = 2.0f * x * y + 2.0f * z * w;
			Float m22 = 1.0f - 2.0f * xx - 2.0f * zz;
			return Vector3(pitch, SeedCore::Atan2(m31, m33), SeedCore::Atan2(m12, m22));
		}

		Float m11 = 1.0f - 2.0f * yy - 2.0f * zz;
		Float m21 = 2.0f * x * y - 2.0f * z * w;
		return Vector3(pitch, 0.0f, SeedCore::Atan2(-m21, m11));
	}

	constexpr Quaternion Quaternion::FromEuler(const Vector3& angles)
	{
		return CreateFromYawPitchRoll(angles.y, angles.x, angles.z);
	}

	constexpr Quaternion Quaternion::CreateFromAxisAngle(const Vector3& axis, Float angle)
	{
		Vector3 normal = axis;
		normal.Normalize();
		Float halfAngle = angle * 0.5f;
		Float sine = SeedCore::Sin(halfAngle);
		return Quaternion(normal.x * sine, normal.y * sine, normal.z * sine, SeedCore::Cos(halfAngle));
	}

	constexpr Quaternion Quaternion::CreateFromYawPitchRoll(Float yaw, Float pitch, Float roll)
	{
		Float halfPitch = pitch * 0.5f;
		Float halfYaw = yaw * 0.5f;
		Float halfRoll = roll * 0.5f;

		Float cosinePitch = SeedCore::Cos(halfPitch);
		Float sinePitch = SeedCore::Sin(halfPitch);
		Float cosineYaw = SeedCore::Cos(halfYaw);
		Float sineYaw = SeedCore::Sin(halfYaw);
		Float cosineRoll = SeedCore::Cos(halfRoll);
		Float sineRoll = SeedCore::Sin(halfRoll);

		return Quaternion(
			cosineRoll * sinePitch * cosineYaw + sineRoll * cosinePitch * sineYaw,
			cosineRoll * cosinePitch * sineYaw - sineRoll * sinePitch * cosineYaw,
			sineRoll * cosinePitch * cosineYaw - cosineRoll * sinePitch * sineYaw,
			cosineRoll * cosinePitch * cosineYaw + sineRoll * sinePitch * sineYaw
			);
	}

	constexpr Quaternion Quaternion::FromToRotation(const Vector3& from, const Vector3& to)
	{
		Vector3 fromNormal = from;
		fromNormal.Normalize();
		Vector3 toNormal = to;
		toNormal.Normalize();

		Float dot = fromNormal.Dot(toNormal);
		if (dot >= 1.0f)
		{
			return Identity;
		}

		if (dot <= -1.0f)
		{
			Vector3 axis = fromNormal.Cross(Vector3::Right);
			if (axis.LengthSq() <= 1.192092896e-7f)
			{
				axis = fromNormal.Cross(Vector3::Up);
			}
			axis.Normalize();
			return Quaternion(axis.x, axis.y, axis.z, 0.0f);
		}

		Vector3 cross = fromNormal.Cross(toNormal);
		Float s = 0.0f;
		if consteval
		{
			s = SeedCore::Sqrt((1.0f + dot) * 2.0f);
		}
		else
		{
			s = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((1.0f + dot) * 2.0f)));
		}
		Float inverse = 1.0f / s;
		return Quaternion(cross.x * inverse, cross.y * inverse, cross.z * inverse, s * 0.5f);
	}

	constexpr Quaternion Quaternion::LookRotation(const Vector3& forward, const Vector3& up)
	{
		Vector3 front = forward;
		front.Normalize();

		Vector3 right = up.Cross(front);
		if (right.LengthSq() <= 1.192092896e-7f)
		{
			return FromToRotation(Vector3::Forward, forward);
		}
		right.Normalize();

		Vector3 upward = front.Cross(right);

		Float m00 = right.x;
		Float m01 = right.y;
		Float m02 = right.z;
		Float m10 = upward.x;
		Float m11 = upward.y;
		Float m12 = upward.z;
		Float m20 = front.x;
		Float m21 = front.y;
		Float m22 = front.z;

		if (m22 <= 0.0f)
		{
			Float difference10 = m11 - m00;
			Float oneMinus22 = 1.0f - m22;
			if (difference10 <= 0.0f)
			{
				Float fourXSq = oneMinus22 - difference10;
				Float inverse = 0.0f;
				if consteval
				{
					inverse = 0.5f / SeedCore::Sqrt(fourXSq);
				}
				else
				{
					inverse = 0.5f / _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(fourXSq)));
				}
				return Quaternion(fourXSq * inverse, (m01 + m10) * inverse, (m02 + m20) * inverse, (m12 - m21) * inverse);
			}
			Float fourYSq = oneMinus22 + difference10;
			Float inverse = 0.0f;
			if consteval
			{
				inverse = 0.5f / SeedCore::Sqrt(fourYSq);
			}
			else
			{
				inverse = 0.5f / _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(fourYSq)));
			}
			return Quaternion((m01 + m10) * inverse, fourYSq * inverse, (m12 + m21) * inverse, (m20 - m02) * inverse);
		}

		Float sum10 = m11 + m00;
		Float onePlus22 = 1.0f + m22;
		if (sum10 <= 0.0f)
		{
			Float fourZSq = onePlus22 - sum10;
			Float inverse = 0.0f;
			if consteval
			{
				inverse = 0.5f / SeedCore::Sqrt(fourZSq);
			}
			else
			{
				inverse = 0.5f / _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(fourZSq)));
			}
			return Quaternion((m02 + m20) * inverse, (m12 + m21) * inverse, fourZSq * inverse, (m01 - m10) * inverse);
		}
		Float fourWSq = onePlus22 + sum10;
		Float inverse = 0.0f;
		if consteval
		{
			inverse = 0.5f / SeedCore::Sqrt(fourWSq);
		}
		else
		{
			inverse = 0.5f / _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(fourWSq)));
		}
		return Quaternion((m12 - m21) * inverse, (m20 - m02) * inverse, (m01 - m10) * inverse, fourWSq * inverse);
	}

	constexpr Quaternion Quaternion::Slerp(const Quaternion& quaternion1, const Quaternion& quaternion2, Float t)
	{
		Float cosine = quaternion1.x * quaternion2.x + quaternion1.y * quaternion2.y + quaternion1.z * quaternion2.z + quaternion1.w * quaternion2.w;
		Float sign = 1.0f;
		if (cosine < 0.0f)
		{
			cosine = -cosine;
			sign = -1.0f;
		}
		Float scale1 = 1.0f - t;
		Float scale2 = t;
		if (cosine < 1.0f - 0.00001f)
		{
			Float sine = 0.0f;
			if consteval
			{
				sine = SeedCore::Sqrt(1.0f - cosine * cosine);
			}
			else
			{
				sine = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(1.0f - cosine * cosine)));
			}
			Float omega = SeedCore::Atan2(sine, cosine);
			Float inverseSine = 1.0f / sine;
			scale1 = SeedCore::Sin(scale1 * omega) * inverseSine;
			scale2 = SeedCore::Sin(scale2 * omega) * inverseSine;
		}
		scale2 *= sign;

		return Quaternion(
			quaternion1.x * scale1 + quaternion2.x * scale2,
			quaternion1.y * scale1 + quaternion2.y * scale2,
			quaternion1.z * scale1 + quaternion2.z * scale2,
			quaternion1.w * scale1 + quaternion2.w * scale2
		);
	}

	constexpr Vector3 Vector3::Transform(const Vector3& vector, const Quaternion& rotation)
	{
		Vector3 axis = Vector3(rotation.x, rotation.y, rotation.z);
		Vector3 twice = axis.Cross(vector) * 2.0f;
		return vector + twice * rotation.w + axis.Cross(twice);
	}

	inline constexpr Quaternion Quaternion::Identity = { 0.0f, 0.0f, 0.0f, 1.0f };
}