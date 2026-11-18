#pragma once

namespace SeedCore
{
	struct Matrix
	{
		union
		{
			struct
			{
				Float _11;
				Float _12;
				Float _13;
				Float _14;
				Float _21;
				Float _22;
				Float _23;
				Float _24;
				Float _31;
				Float _32;
				Float _33;
				Float _34;
				Float _41;
				Float _42;
				Float _43;
				Float _44;
			};
			struct
			{
				Float m[4][4];
			};
			struct
			{
				Vector4 r[4];
			};
		};

		constexpr Matrix();
		constexpr Matrix(Float m00, Float m01, Float m02, Float m03, Float m10, Float m11, Float m12, Float m13, Float m20, Float m21, Float m22, Float m23, Float m30, Float m31, Float m32, Float m33);

		constexpr Bool operator==(const Matrix& matrix)const;
		constexpr Bool operator!=(const Matrix& matrix)const;

		constexpr Matrix& operator*=(const Matrix& matrix);
		constexpr Matrix operator*(const Matrix& matrix)const;

		NODISCARD constexpr Vector3 Up()const;
		NODISCARD constexpr Vector3 Down()const;
		NODISCARD constexpr Vector3 Right()const;
		NODISCARD constexpr Vector3 Left()const;
		NODISCARD constexpr Vector3 Forward()const;
		NODISCARD constexpr Vector3 Backward()const;

		NODISCARD constexpr Vector3 Translation()const;
		NODISCARD constexpr Quaternion Rotation()const;
		NODISCARD constexpr Vector3 Scale()const;

		NODISCARD constexpr Matrix Transpose()const;
		NODISCARD constexpr Matrix Invert()const;

		NODISCARD constexpr Float Determinant()const;

		constexpr Bool Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale)const;

		NODISCARD static constexpr Matrix CreateTranslation(const Vector3& translation);
		NODISCARD static constexpr Matrix CreateTranslation(Float x, Float y, Float z);

		NODISCARD static constexpr Matrix CreateFromAxisAngle(const Vector3& axis, Float angle);
		NODISCARD static constexpr Matrix CreateFromQuaternion(const Quaternion& rotation);
		NODISCARD static constexpr Matrix CreateFromYawPitchRoll(Float yaw, Float pitch, Float roll);
		NODISCARD static constexpr Matrix CreateFromEuler(const Vector3& angles);

		NODISCARD static constexpr Matrix CreateScale(const Vector3& scale);
		NODISCARD static constexpr Matrix CreateScale(Float x, Float y, Float z);

		NODISCARD static constexpr Matrix CreateAffineTransformation(const Vector3& translation, const Quaternion& rotation, const Vector3& scale, const Vector3& pivot);

		NODISCARD static constexpr Matrix CreateTranslationRotationScale(const Vector3& translation, const Quaternion& rotation, const Vector3& scale);
		NODISCARD static constexpr Matrix CreateTranslationForwardUp(const Vector3& translation, const Vector3& forward, const Vector3& up);

		NODISCARD static constexpr Matrix CreateViewLookAt(const Vector3& eye, const Vector3& target, const Vector3& up);

		NODISCARD static constexpr Matrix CreatePerspective(Float width, Float height, Float nearPlane, Float farPlane);
		NODISCARD static constexpr Matrix CreateOrthographic(Float width, Float height, Float nearPlane, Float farPlane);

		NODISCARD static constexpr Matrix CreatePerspectiveFov(Float fov, Float aspectRatio, Float nearPlane, Float farPlane);
		NODISCARD static constexpr Matrix CreateOrthographicOffCenter(Float left, Float right, Float bottom, Float top, Float nearPlane, Float farPlane);

		static const Matrix Identity;
	};

	constexpr Matrix::Matrix() :m{ { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } }
	{
		/// No Code
	}

	constexpr Matrix::Matrix(Float m00, Float m01, Float m02, Float m03, Float m10, Float m11, Float m12, Float m13, Float m20, Float m21, Float m22, Float m23, Float m30, Float m31, Float m32, Float m33) : m{ { m00, m01, m02, m03 }, { m10, m11, m12, m13 }, { m20, m21, m22, m23 }, { m30, m31, m32, m33 } }
	{
		/// No Code
	}

	constexpr Bool Matrix::operator==(const Matrix& matrix)const
	{
		for (Int rowIndex = 0;rowIndex < 4;++rowIndex)
		{
			for (Int columnIndex = 0;columnIndex < 4;++columnIndex)
			{
				if (m[rowIndex][columnIndex] != matrix.m[rowIndex][columnIndex])
				{
					return false;
				}
			}
		}
		return true;
	}

	constexpr Bool Matrix::operator!=(const Matrix& matrix)const
	{
		return !(*this == matrix);
	}

	constexpr Matrix& Matrix::operator*=(const Matrix& matrix)
	{
		*this = *this * matrix;
		return *this;
	}

	constexpr Matrix Matrix::operator*(const Matrix& matrix)const
	{
		Matrix result;
		if consteval
		{
			for (Int rowIndex = 0;rowIndex < 4;++rowIndex)
			{
				for (Int columnIndex = 0;columnIndex < 4;++columnIndex)
				{
					result.m[rowIndex][columnIndex] = m[rowIndex][0] * matrix.m[0][columnIndex] + m[rowIndex][1] * matrix.m[1][columnIndex] + m[rowIndex][2] * matrix.m[2][columnIndex] + m[rowIndex][3] * matrix.m[3][columnIndex];
				}
			}
		}
		else
		{
			__m128 row0 = _mm_loadu_ps(matrix.m[0]);
			__m128 row1 = _mm_loadu_ps(matrix.m[1]);
			__m128 row2 = _mm_loadu_ps(matrix.m[2]);
			__m128 row3 = _mm_loadu_ps(matrix.m[3]);
			for (Int rowIndex = 0;rowIndex < 4;++rowIndex)
			{
				__m128 sum = _mm_mul_ps(_mm_set1_ps(m[rowIndex][0]), row0);
				sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(m[rowIndex][1]), row1));
				sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(m[rowIndex][2]), row2));
				sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(m[rowIndex][3]), row3));
				_mm_storeu_ps(result.m[rowIndex], sum);
			}
		}
		return result;
	}

	constexpr Vector3 Matrix::Up()const
	{
		return Vector3(m[1][0], m[1][1], m[1][2]);
	}

	constexpr Vector3 Matrix::Down()const
	{
		return Vector3(-m[1][0], -m[1][1], -m[1][2]);
	}

	constexpr Vector3 Matrix::Right()const
	{
		return Vector3(m[0][0], m[0][1], m[0][2]);
	}

	constexpr Vector3 Matrix::Left()const
	{
		return Vector3(-m[0][0], -m[0][1], -m[0][2]);
	}

	constexpr Vector3 Matrix::Forward()const
	{
		return Vector3(m[2][0], m[2][1], m[2][2]);
	}

	constexpr Vector3 Matrix::Backward()const
	{
		return Vector3(-m[2][0], -m[2][1], -m[2][2]);
	}

	constexpr Vector3 Matrix::Translation()const
	{
		return Vector3(m[3][0], m[3][1], m[3][2]);
	}

	constexpr Quaternion Matrix::Rotation()const
	{
		Vector3 translation, scale;
		Quaternion rotation;

		if (!Decompose(translation, rotation, scale))
		{
			return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}

		return rotation;
	}

	constexpr Vector3 Matrix::Scale()const
	{
		return Vector3(Vector3(m[0][0], m[0][1], m[0][2]).Length(), Vector3(m[1][0], m[1][1], m[1][2]).Length(), Vector3(m[2][0], m[2][1], m[2][2]).Length());
	}

	constexpr Matrix Matrix::Transpose()const
	{
		Matrix result;
		if consteval
		{
			for (Int rowIndex = 0;rowIndex < 4;++rowIndex)
			{
				for (Int columnIndex = 0;columnIndex < 4;++columnIndex)
				{
					result.m[rowIndex][columnIndex] = m[columnIndex][rowIndex];
				}
			}
		}
		else
		{
			__m128 row0 = _mm_loadu_ps(m[0]);
			__m128 row1 = _mm_loadu_ps(m[1]);
			__m128 row2 = _mm_loadu_ps(m[2]);
			__m128 row3 = _mm_loadu_ps(m[3]);
			__m128 low01 = _mm_shuffle_ps(row0, row1, _MM_SHUFFLE(1, 0, 1, 0));
			__m128 high01 = _mm_shuffle_ps(row0, row1, _MM_SHUFFLE(3, 2, 3, 2));
			__m128 low23 = _mm_shuffle_ps(row2, row3, _MM_SHUFFLE(1, 0, 1, 0));
			__m128 high23 = _mm_shuffle_ps(row2, row3, _MM_SHUFFLE(3, 2, 3, 2));
			_mm_storeu_ps(result.m[0], _mm_shuffle_ps(low01, low23, _MM_SHUFFLE(2, 0, 2, 0)));
			_mm_storeu_ps(result.m[1], _mm_shuffle_ps(low01, low23, _MM_SHUFFLE(3, 1, 3, 1)));
			_mm_storeu_ps(result.m[2], _mm_shuffle_ps(high01, high23, _MM_SHUFFLE(2, 0, 2, 0)));
			_mm_storeu_ps(result.m[3], _mm_shuffle_ps(high01, high23, _MM_SHUFFLE(3, 1, 3, 1)));
		}
		return result;
	}

	constexpr Matrix Matrix::Invert()const
	{
		Matrix result;
		if consteval
		{
			Float s0 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
			Float s1 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
			Float s2 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
			Float s3 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
			Float s4 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
			Float s5 = m[0][2] * m[1][3] - m[1][2] * m[0][3];
			Float c5 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
			Float c4 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
			Float c3 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
			Float c2 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
			Float c1 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
			Float c0 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

			Float inverse = 1.0f / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);
			result.m[0][0] = (m[1][1] * c5 - m[1][2] * c4 + m[1][3] * c3) * inverse;
			result.m[0][1] = (-m[0][1] * c5 + m[0][2] * c4 - m[0][3] * c3) * inverse;
			result.m[0][2] = (m[3][1] * s5 - m[3][2] * s4 + m[3][3] * s3) * inverse;
			result.m[0][3] = (-m[2][1] * s5 + m[2][2] * s4 - m[2][3] * s3) * inverse;
			result.m[1][0] = (-m[1][0] * c5 + m[1][2] * c2 - m[1][3] * c1) * inverse;
			result.m[1][1] = (m[0][0] * c5 - m[0][2] * c2 + m[0][3] * c1) * inverse;
			result.m[1][2] = (-m[3][0] * s5 + m[3][2] * s2 - m[3][3] * s1) * inverse;
			result.m[1][3] = (m[2][0] * s5 - m[2][2] * s2 + m[2][3] * s1) * inverse;
			result.m[2][0] = (m[1][0] * c4 - m[1][1] * c2 + m[1][3] * c0) * inverse;
			result.m[2][1] = (-m[0][0] * c4 + m[0][1] * c2 - m[0][3] * c0) * inverse;
			result.m[2][2] = (m[3][0] * s4 - m[3][1] * s2 + m[3][3] * s0) * inverse;
			result.m[2][3] = (-m[2][0] * s4 + m[2][1] * s2 - m[2][3] * s0) * inverse;
			result.m[3][0] = (-m[1][0] * c3 + m[1][1] * c1 - m[1][2] * c0) * inverse;
			result.m[3][1] = (m[0][0] * c3 - m[0][1] * c1 + m[0][2] * c0) * inverse;
			result.m[3][2] = (-m[3][0] * s3 + m[3][1] * s1 - m[3][2] * s0) * inverse;
			result.m[3][3] = (m[2][0] * s3 - m[2][1] * s1 + m[2][2] * s0) * inverse;
		}
		else
		{
			const __m128 r0 = _mm_loadu_ps(m[0]);
			const __m128 r1 = _mm_loadu_ps(m[1]);
			const __m128 r2 = _mm_loadu_ps(m[2]);
			const __m128 r3 = _mm_loadu_ps(m[3]);
			const __m128 A = _mm_movelh_ps(r0, r1);
			const __m128 B = _mm_movehl_ps(r1, r0);
			const __m128 C = _mm_movelh_ps(r2, r3);
			const __m128 D = _mm_movehl_ps(r3, r2);

			auto adj2 = [](__m128 x) -> __m128
			{
				const __m128 swapped = _mm_shuffle_ps(x, x, _MM_SHUFFLE(0, 2, 1, 3));
				const __m128 signs = _mm_setr_ps(1.0f, -1.0f, -1.0f, 1.0f);
				return _mm_mul_ps(swapped, signs);
			};

			auto mul2 = [](__m128 x, __m128 y) -> __m128
			{
				const __m128 x0 = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2, 2, 0, 0));
				const __m128 x1 = _mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 3, 1, 1));
				const __m128 y0 = _mm_shuffle_ps(y, y, _MM_SHUFFLE(1, 0, 1, 0));
				const __m128 y1 = _mm_shuffle_ps(y, y, _MM_SHUFFLE(3, 2, 3, 2));
				return _mm_add_ps(_mm_mul_ps(x0, y0), _mm_mul_ps(x1, y1));
			};

			auto det2 = [](__m128 x) -> Float
				{
					const __m128 rev = _mm_shuffle_ps(x, x, _MM_SHUFFLE(0, 1, 2, 3));
					const __m128 prod = _mm_mul_ps(x, rev);
					return _mm_cvtss_f32(prod) - _mm_cvtss_f32(_mm_shuffle_ps(prod, prod, _MM_SHUFFLE(1, 1, 1, 1)));
				};

			const Float a = det2(A);
			const Float b = det2(B);
			const Float c = det2(C);
			const Float d = det2(D);
			const __m128 AB = mul2(adj2(A), B);
			const __m128 DC = mul2(adj2(D), C);
			const __m128 dot = mul2(AB, DC);
			const Float trace = _mm_cvtss_f32(dot) + _mm_cvtss_f32(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(3, 3, 3, 3)));
			const Float det = a * d + b * c - trace;
			const __m128 reciprocalDet = _mm_set1_ps(1.0f / det);
			const __m128 X = _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(d), A), mul2(B, DC));
			const __m128 W = _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(a), D), mul2(C, AB));
			const __m128 Y = _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(b), C), mul2(D, adj2(AB)));
			const __m128 Z = _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(c), B), mul2(A, adj2(DC)));
			const __m128 iA = _mm_mul_ps(adj2(X), reciprocalDet);
			const __m128 iB = _mm_mul_ps(adj2(Y), reciprocalDet);
			const __m128 iC = _mm_mul_ps(adj2(Z), reciprocalDet);
			const __m128 iD = _mm_mul_ps(adj2(W), reciprocalDet);
			_mm_storeu_ps(result.m[0], _mm_movelh_ps(iA, iB));
			_mm_storeu_ps(result.m[1], _mm_movehl_ps(iB, iA));
			_mm_storeu_ps(result.m[2], _mm_movelh_ps(iC, iD));
			_mm_storeu_ps(result.m[3], _mm_movehl_ps(iD, iC));
		}
		return result;
	}

	constexpr Float Matrix::Determinant()const
	{
		Float s0 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
		Float s1 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
		Float s2 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
		Float s3 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
		Float s4 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
		Float s5 = m[0][2] * m[1][3] - m[1][2] * m[0][3];
		Float c5 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
		Float c4 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
		Float c3 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
		Float c2 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
		Float c1 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
		Float c0 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
		return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
	}

	constexpr Bool Matrix::Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale)const
	{
		translation = Vector3(m[3][0], m[3][1], m[3][2]);

		Vector3 canonical[3] = { Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ };
		Vector3 basis[3] = { Vector3(m[0][0], m[0][1], m[0][2]), Vector3(m[1][0], m[1][1], m[1][2]), Vector3(m[2][0], m[2][1], m[2][2]) };
		Float scales[3] = { basis[0].Length(), basis[1].Length(), basis[2].Length() };

		Int a = 0;
		Int b = 0;
		Int c = 0;
		if (scales[0] < scales[1])
		{
			if (scales[1] < scales[2])
			{
				a = 2;
				b = 1;
				c = 0;
			}
			else
			{
				a = 1;
				b = scales[0] < scales[2] ? 2 : 0;
				c = scales[0] < scales[2] ? 0 : 2;
			}
		}
		else
		{
			if (scales[0] < scales[2])
			{
				a = 2;
				b = 0;
				c = 1;
			}
			else
			{
				a = 0;
				b = scales[1] < scales[2] ? 2 : 1;
				c = scales[1] < scales[2] ? 1 : 2;
			}
		}

		if (scales[a] < 0.0001f)
		{
			basis[a] = canonical[a];
		}
		basis[a].Normalize();

		if (scales[b] < 0.0001f)
		{
			Float absoluteX = basis[a].x < 0.0f ? -basis[a].x : basis[a].x;
			Float absoluteY = basis[a].y < 0.0f ? -basis[a].y : basis[a].y;
			Float absoluteZ = basis[a].z < 0.0f ? -basis[a].z : basis[a].z;
			Int smallest = 0;
			if (absoluteX < absoluteY)
			{
				smallest = (absoluteY < absoluteZ || absoluteX < absoluteZ) ? 0 : 2;
			}
			else
			{
				smallest = (absoluteX < absoluteZ || absoluteY < absoluteZ) ? 1 : 2;
			}
			basis[b] = basis[a].Cross(canonical[smallest]);
		}
		basis[b].Normalize();

		if (scales[c] < 0.0001f)
		{
			basis[c] = basis[a].Cross(basis[b]);
		}
		basis[c].Normalize();

		Float determinant = basis[0].Dot(basis[1].Cross(basis[2]));
		if (determinant < 0.0f)
		{
			scales[a] = -scales[a];
			basis[a] = -basis[a];
			determinant = -determinant;
		}

		determinant -= 1.0f;
		determinant *= determinant;
		if (0.0001f < determinant)
		{
			return false;
		}

		scale = Vector3(scales[0], scales[1], scales[2]);
		rotation = Quaternion::CreateFromRotationMatrix(Matrix(basis[0].x, basis[0].y, basis[0].z, 0.0f, basis[1].x, basis[1].y, basis[1].z, 0.0f, basis[2].x, basis[2].y, basis[2].z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
		return true;
	}

	constexpr Matrix Matrix::CreateTranslation(const Vector3& translation)
	{
		return Matrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, translation.x, translation.y, translation.z, 1.0f);
	}

	constexpr Matrix Matrix::CreateTranslation(Float x, Float y, Float z)
	{
		return Matrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, x, y, z, 1.0f);
	}

	constexpr Matrix Matrix::CreateFromAxisAngle(const Vector3& axis, Float angle)
	{
		Vector3 normal = axis;
		normal.Normalize();
		Float sine = SeedCore::Sin(angle);
		Float cosine = SeedCore::Cos(angle);
		Float oneMinusCosine = 1.0f - cosine;
		Float xy = normal.x * normal.y * oneMinusCosine;
		Float yz = normal.y * normal.z * oneMinusCosine;
		Float zx = normal.z * normal.x * oneMinusCosine;
		return Matrix(cosine + normal.x * normal.x * oneMinusCosine, xy + normal.z * sine, zx - normal.y * sine, 0.0f, xy - normal.z * sine, cosine + normal.y * normal.y * oneMinusCosine, yz + normal.x * sine, 0.0f, zx + normal.y * sine, yz - normal.x * sine, cosine + normal.z * normal.z * oneMinusCosine, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	constexpr Matrix Matrix::CreateFromQuaternion(const Quaternion& rotation)
	{
		Float xx = rotation.x * rotation.x;
		Float yy = rotation.y * rotation.y;
		Float zz = rotation.z * rotation.z;
		Float xy = rotation.x * rotation.y;
		Float xz = rotation.x * rotation.z;
		Float yz = rotation.y * rotation.z;
		Float xw = rotation.x * rotation.w;
		Float yw = rotation.y * rotation.w;
		Float zw = rotation.z * rotation.w;
		return Matrix(1.0f - 2.0f * yy - 2.0f * zz, 2.0f * xy + 2.0f * zw, 2.0f * xz - 2.0f * yw, 0.0f, 2.0f * xy - 2.0f * zw, 1.0f - 2.0f * xx - 2.0f * zz, 2.0f * yz + 2.0f * xw, 0.0f, 2.0f * xz + 2.0f * yw, 2.0f * yz - 2.0f * xw, 1.0f - 2.0f * xx - 2.0f * yy, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	constexpr Matrix Matrix::CreateFromYawPitchRoll(Float yaw, Float pitch, Float roll)
	{
		Float cosineYaw = SeedCore::Cos(yaw);
		Float sineYaw = SeedCore::Sin(yaw);
		Float cosinePitch = SeedCore::Cos(pitch);
		Float sinePitch = SeedCore::Sin(pitch);
		Float cosineRoll = SeedCore::Cos(roll);
		Float sineRoll = SeedCore::Sin(roll);
		return Matrix(cosineRoll * cosineYaw + sineRoll * sinePitch * sineYaw, sineRoll * cosinePitch, sineRoll * sinePitch * cosineYaw - cosineRoll * sineYaw, 0.0f, cosineRoll * sinePitch * sineYaw - sineRoll * cosineYaw, cosineRoll * cosinePitch, sineRoll * sineYaw + cosineRoll * sinePitch * cosineYaw, 0.0f, cosinePitch * sineYaw, -sinePitch, cosinePitch * cosineYaw, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	constexpr Matrix Matrix::CreateFromEuler(const Vector3& angles)
	{
		return CreateFromYawPitchRoll(angles.y, angles.x, angles.z);
	}

	constexpr Matrix Matrix::CreateScale(const Vector3& scale)
	{
		return Matrix(scale.x, 0.0f, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 0.0f, scale.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	constexpr Matrix Matrix::CreateScale(Float x, Float y, Float z)
	{
		return Matrix(x, 0.0f, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 0.0f, z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	constexpr Matrix Matrix::CreateAffineTransformation(const Vector3& translation, const Quaternion& rotation, const Vector3& scale, const Vector3& pivot)
	{
		Matrix result = CreateTranslationRotationScale(translation, rotation, scale);

		Vector3 transformedPivot(pivot.x * result._11 + pivot.y * result._21 + pivot.z * result._31, pivot.x * result._12 + pivot.y * result._22 + pivot.z * result._32, pivot.x * result._13 + pivot.y * result._23 + pivot.z * result._33);

		result._41 += pivot.x - transformedPivot.x;
		result._42 += pivot.y - transformedPivot.y;
		result._43 += pivot.z - transformedPivot.z;

		return result;
	}

	constexpr Matrix Matrix::CreateTranslationRotationScale(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
	{
		Float xx = rotation.x * rotation.x;
		Float yy = rotation.y * rotation.y;
		Float zz = rotation.z * rotation.z;
		Float xy = rotation.x * rotation.y;
		Float xz = rotation.x * rotation.z;
		Float yz = rotation.y * rotation.z;
		Float xw = rotation.x * rotation.w;
		Float yw = rotation.y * rotation.w;
		Float zw = rotation.z * rotation.w;
		return Matrix((1.0f - 2.0f * (yy + zz)) * scale.x, (2.0f * (xy + zw)) * scale.x, (2.0f * (xz - yw)) * scale.x, 0.0f, (2.0f * (xy - zw)) * scale.y, (1.0f - 2.0f * (xx + zz)) * scale.y, (2.0f * (yz + xw)) * scale.y, 0.0f, (2.0f * (xz + yw)) * scale.z, (2.0f * (yz - xw)) * scale.z, (1.0f - 2.0f * (xx + yy)) * scale.z, 0.0f, translation.x, translation.y, translation.z, 1.0f);
	}

	constexpr Matrix Matrix::CreateTranslationForwardUp(const Vector3& translation, const Vector3& forward, const Vector3& up)
	{
		Vector3 axisZ = forward;
		axisZ.Normalize();
		Vector3 axisX = up.Cross(axisZ);
		axisX.Normalize();
		Vector3 axisY = axisZ.Cross(axisX);
		return Matrix(axisX.x, axisX.y, axisX.z, 0.0f, axisY.x, axisY.y, axisY.z, 0.0f, axisZ.x, axisZ.y, axisZ.z, 0.0f, translation.x, translation.y, translation.z, 1.0f);
	}

	constexpr Matrix Matrix::CreateViewLookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
	{
		Matrix result;
		if consteval
		{
			Vector3 axisZ = target - eye;
			axisZ.Normalize();
			Vector3 axisX = up.Cross(axisZ);
			axisX.Normalize();
			Vector3 axisY = axisZ.Cross(axisX);
			result = Matrix(axisX.x, axisY.x, axisZ.x, 0.0f, axisX.y, axisY.y, axisZ.y, 0.0f, axisX.z, axisY.z, axisZ.z, 0.0f, -axisX.Dot(eye), -axisY.Dot(eye), -axisZ.Dot(eye), 1.0f);
		}
		else
		{
			__m128 eyeVector = _mm_setr_ps(eye.x, eye.y, eye.z, 0.0f);
			__m128 upVector = _mm_setr_ps(up.x, up.y, up.z, 0.0f);

			__m128 axisZ = _mm_sub_ps(_mm_setr_ps(target.x, target.y, target.z, 0.0f), eyeVector);
			__m128 squaredZ = _mm_mul_ps(axisZ, axisZ);
			__m128 lengthSqZ = _mm_add_ss(_mm_add_ss(squaredZ, _mm_shuffle_ps(squaredZ, squaredZ, _MM_SHUFFLE(1, 1, 1, 1))), _mm_shuffle_ps(squaredZ, squaredZ, _MM_SHUFFLE(2, 2, 2, 2)));
			lengthSqZ = _mm_shuffle_ps(lengthSqZ, lengthSqZ, _MM_SHUFFLE(0, 0, 0, 0));
			axisZ = _mm_and_ps(_mm_div_ps(axisZ, _mm_sqrt_ps(lengthSqZ)), _mm_cmpgt_ps(lengthSqZ, _mm_setzero_ps()));

			__m128 axisX = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(upVector, upVector, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(axisZ, axisZ, _MM_SHUFFLE(3, 1, 0, 2))), _mm_mul_ps(_mm_shuffle_ps(upVector, upVector, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(axisZ, axisZ, _MM_SHUFFLE(3, 0, 2, 1))));
			__m128 squaredX = _mm_mul_ps(axisX, axisX);
			__m128 lengthSqX = _mm_add_ss(_mm_add_ss(squaredX, _mm_shuffle_ps(squaredX, squaredX, _MM_SHUFFLE(1, 1, 1, 1))), _mm_shuffle_ps(squaredX, squaredX, _MM_SHUFFLE(2, 2, 2, 2)));
			lengthSqX = _mm_shuffle_ps(lengthSqX, lengthSqX, _MM_SHUFFLE(0, 0, 0, 0));
			axisX = _mm_and_ps(_mm_div_ps(axisX, _mm_sqrt_ps(lengthSqX)), _mm_cmpgt_ps(lengthSqX, _mm_setzero_ps()));

			__m128 axisY = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(axisZ, axisZ, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(axisX, axisX, _MM_SHUFFLE(3, 1, 0, 2))), _mm_mul_ps(_mm_shuffle_ps(axisZ, axisZ, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(axisX, axisX, _MM_SHUFFLE(3, 0, 2, 1))));

			__m128 lowXY = _mm_shuffle_ps(axisX, axisY, _MM_SHUFFLE(1, 0, 1, 0));
			__m128 highXY = _mm_shuffle_ps(axisX, axisY, _MM_SHUFFLE(3, 2, 3, 2));
			__m128 lowZ = _mm_shuffle_ps(axisZ, _mm_setzero_ps(), _MM_SHUFFLE(1, 0, 1, 0));
			__m128 highZ = _mm_shuffle_ps(axisZ, _mm_setzero_ps(), _MM_SHUFFLE(3, 2, 3, 2));
			__m128 row0 = _mm_shuffle_ps(lowXY, lowZ, _MM_SHUFFLE(2, 0, 2, 0));
			__m128 row1 = _mm_shuffle_ps(lowXY, lowZ, _MM_SHUFFLE(3, 1, 3, 1));
			__m128 row2 = _mm_shuffle_ps(highXY, highZ, _MM_SHUFFLE(2, 0, 2, 0));

			__m128 rotatedEye = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(eye.x), row0), _mm_mul_ps(_mm_set1_ps(eye.y), row1)), _mm_mul_ps(_mm_set1_ps(eye.z), row2));

			_mm_storeu_ps(result.m[0], row0);
			_mm_storeu_ps(result.m[1], row1);
			_mm_storeu_ps(result.m[2], row2);
			_mm_storeu_ps(result.m[3], _mm_sub_ps(_mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f), rotatedEye));
		}
		return result;
	}

	constexpr Matrix Matrix::CreatePerspective(Float width, Float height, Float nearPlane, Float farPlane)
	{
		Float infinity = std::numeric_limits<Float>::infinity();
		Float nearDistance = nearPlane < farPlane ? nearPlane : farPlane;
		Float depthScale = 0.0f;
		Float depthOffset = 0.0f;
		if (farPlane == infinity)
		{
			depthScale = 1.0f;
			depthOffset = -nearPlane;
		}
		else if (nearPlane == infinity)
		{
			depthScale = 0.0f;
			depthOffset = farPlane;
		}
		else
		{
			depthScale = farPlane / (farPlane - nearPlane);
			depthOffset = -depthScale * nearPlane;
		}

		Matrix result;
		if consteval
		{
			result = Matrix(2.0f * nearDistance / width, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f * nearDistance / height, 0.0f, 0.0f, 0.0f, 0.0f, depthScale, 1.0f, 0.0f, 0.0f, depthOffset, 0.0f);
		}
		else
		{
			__m128 scale = _mm_div_ps(_mm_set1_ps(2.0f * nearDistance), _mm_setr_ps(width, height, 1.0f, 1.0f));
			_mm_storeu_ps(result.m[0], _mm_and_ps(scale, _mm_castsi128_ps(_mm_setr_epi32(-1, 0, 0, 0))));
			_mm_storeu_ps(result.m[1], _mm_and_ps(scale, _mm_castsi128_ps(_mm_setr_epi32(0, -1, 0, 0))));
			_mm_storeu_ps(result.m[2], _mm_setr_ps(0.0f, 0.0f, depthScale, 1.0f));
			_mm_storeu_ps(result.m[3], _mm_setr_ps(0.0f, 0.0f, depthOffset, 0.0f));
		}
		return result;
	}

	constexpr Matrix Matrix::CreateOrthographic(Float width, Float height, Float nearPlane, Float farPlane)
	{
		Float range = 1.0f / (farPlane - nearPlane);
		return Matrix(2.0f / width, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f / height, 0.0f, 0.0f, 0.0f, 0.0f, range, 0.0f, 0.0f, 0.0f, -range * nearPlane, 1.0f);
	}

	constexpr Matrix Matrix::CreatePerspectiveFov(Float fov, Float aspectRatio, Float nearPlane, Float farPlane)
	{
		Float halfAngle = fov * 0.5f;
		Float halfAngleSq = halfAngle * halfAngle;
		Float sine = (((((-2.3889859e-08f * halfAngleSq + 2.7525562e-06f) * halfAngleSq - 0.00019840874f) * halfAngleSq + 0.0083333310f) * halfAngleSq - 0.16666667f) * halfAngleSq + 1.0f) * halfAngle;
		Float cosine = ((((-2.6051615e-07f * halfAngleSq + 2.4760495e-05f) * halfAngleSq - 0.0013888378f) * halfAngleSq + 0.041666638f) * halfAngleSq - 0.5f) * halfAngleSq + 1.0f;

		Float infinity = std::numeric_limits<Float>::infinity();
		Float depthScale = 0.0f;
		Float depthOffset = 0.0f;
		if (farPlane == infinity)
		{
			depthScale = 1.0f;
			depthOffset = -nearPlane;
		}
		else if (nearPlane == infinity)
		{
			depthScale = 0.0f;
			depthOffset = farPlane;
		}
		else
		{
			depthScale = farPlane / (farPlane - nearPlane);
			depthOffset = -depthScale * nearPlane;
		}

		Matrix result;
		if consteval
		{
			Float height = cosine / sine;
			result = Matrix(height / aspectRatio, 0.0f, 0.0f, 0.0f, 0.0f, height, 0.0f, 0.0f, 0.0f, 0.0f, depthScale, 1.0f, 0.0f, 0.0f, depthOffset, 0.0f);
		}
		else
		{
			__m128 scale = _mm_div_ps(_mm_set1_ps(cosine), _mm_setr_ps(sine * aspectRatio, sine, 1.0f, 1.0f));
			_mm_storeu_ps(result.m[0], _mm_and_ps(scale, _mm_castsi128_ps(_mm_setr_epi32(-1, 0, 0, 0))));
			_mm_storeu_ps(result.m[1], _mm_and_ps(scale, _mm_castsi128_ps(_mm_setr_epi32(0, -1, 0, 0))));
			_mm_storeu_ps(result.m[2], _mm_setr_ps(0.0f, 0.0f, depthScale, 1.0f));
			_mm_storeu_ps(result.m[3], _mm_setr_ps(0.0f, 0.0f, depthOffset, 0.0f));
		}
		return result;
	}

	constexpr Matrix Matrix::CreateOrthographicOffCenter(Float left, Float right, Float bottom, Float top, Float nearPlane, Float farPlane)
	{
		Float inverseWidth = 1.0f / (right - left);
		Float inverseHeight = 1.0f / (top - bottom);
		Float range = 1.0f / (farPlane - nearPlane);
		return Matrix(inverseWidth + inverseWidth, 0.0f, 0.0f, 0.0f, 0.0f, inverseHeight + inverseHeight, 0.0f, 0.0f, 0.0f, 0.0f, range, 0.0f, -(left + right) * inverseWidth, -(top + bottom) * inverseHeight, -range * nearPlane, 1.0f);
	}

	constexpr Vector3 Vector3::TransformPosition(const Vector3& position, const Matrix& matrix)
	{
		Vector3 result;
		if consteval
		{
			result = Vector3(position.x * matrix.m[0][0] + position.y * matrix.m[1][0] + position.z * matrix.m[2][0] + matrix.m[3][0], position.x * matrix.m[0][1] + position.y * matrix.m[1][1] + position.z * matrix.m[2][1] + matrix.m[3][1], position.x * matrix.m[0][2] + position.y * matrix.m[1][2] + position.z * matrix.m[2][2] + matrix.m[3][2]);
		}
		else
		{
			__m128 sum = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(position.x), _mm_loadu_ps(matrix.m[0])), _mm_loadu_ps(matrix.m[3]));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(position.y), _mm_loadu_ps(matrix.m[1])));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(position.z), _mm_loadu_ps(matrix.m[2])));
			_mm_storel_pi(reinterpret_cast<__m64*>(&result.x), sum);
			_mm_store_ss(&result.z, _mm_movehl_ps(sum, sum));
		}
		return result;
	}

	constexpr Vector3 Vector3::TransformCoordinate(const Vector3& coordinate, const Matrix& matrix)
	{
		Vector3 result;
		if consteval
		{
			Float inverseW = 1.0f / (coordinate.x * matrix.m[0][3] + coordinate.y * matrix.m[1][3] + coordinate.z * matrix.m[2][3] + matrix.m[3][3]);
			result = Vector3((coordinate.x * matrix.m[0][0] + coordinate.y * matrix.m[1][0] + coordinate.z * matrix.m[2][0] + matrix.m[3][0]) * inverseW, (coordinate.x * matrix.m[0][1] + coordinate.y * matrix.m[1][1] + coordinate.z * matrix.m[2][1] + matrix.m[3][1]) * inverseW, (coordinate.x * matrix.m[0][2] + coordinate.y * matrix.m[1][2] + coordinate.z * matrix.m[2][2] + matrix.m[3][2]) * inverseW);
		}
		else
		{
			__m128 sum = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(coordinate.x), _mm_loadu_ps(matrix.m[0])), _mm_loadu_ps(matrix.m[3]));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(coordinate.y), _mm_loadu_ps(matrix.m[1])));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(coordinate.z), _mm_loadu_ps(matrix.m[2])));
			sum = _mm_div_ps(sum, _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(3, 3, 3, 3)));
			_mm_storel_pi(reinterpret_cast<__m64*>(&result.x), sum);
			_mm_store_ss(&result.z, _mm_movehl_ps(sum, sum));
		}
		return result;
	}

	constexpr Vector3 Vector3::TransformDirection(const Vector3& direction, const Matrix& matrix)
	{
		Vector3 result;
		if consteval
		{
			result = Vector3(direction.x * matrix.m[0][0] + direction.y * matrix.m[1][0] + direction.z * matrix.m[2][0], direction.x * matrix.m[0][1] + direction.y * matrix.m[1][1] + direction.z * matrix.m[2][1], direction.x * matrix.m[0][2] + direction.y * matrix.m[1][2] + direction.z * matrix.m[2][2]);
		}
		else
		{
			__m128 sum = _mm_mul_ps(_mm_set1_ps(direction.x), _mm_loadu_ps(matrix.m[0]));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(direction.y), _mm_loadu_ps(matrix.m[1])));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(direction.z), _mm_loadu_ps(matrix.m[2])));
			_mm_storel_pi(reinterpret_cast<__m64*>(&result.x), sum);
			_mm_store_ss(&result.z, _mm_movehl_ps(sum, sum));
		}
		return result;
	}

	constexpr Vector3 Vector3::TransformNormal(const Vector3& normal, const Matrix& matrix)
	{
		Vector3 row0 = Vector3(matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]);
		Vector3 row1 = Vector3(matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]);
		Vector3 row2 = Vector3(matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]);
		Vector3 cofactor0 = row1.Cross(row2);
		Vector3 cofactor1 = row2.Cross(row0);
		Vector3 cofactor2 = row0.Cross(row1);
		Vector3 result = cofactor0 * normal.x + cofactor1 * normal.y + cofactor2 * normal.z;
		if (row0.Dot(cofactor0) < 0.0f)
		{
			result = -result;
		}
		result.Normalize();
		return result;
	}

	constexpr Vector4 Vector4::Transform(const Vector4& vector, const Matrix& matrix)
	{
		Vector4 result;
		if consteval
		{
			result = Vector4(vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + vector.w * matrix.m[3][0], vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + vector.w * matrix.m[3][1], vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + vector.w * matrix.m[3][2], vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + vector.w * matrix.m[3][3]);
		}
		else
		{
			__m128 sum = _mm_mul_ps(_mm_set1_ps(vector.x), _mm_loadu_ps(matrix.m[0]));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(vector.y), _mm_loadu_ps(matrix.m[1])));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(vector.z), _mm_loadu_ps(matrix.m[2])));
			sum = _mm_add_ps(sum, _mm_mul_ps(_mm_set1_ps(vector.w), _mm_loadu_ps(matrix.m[3])));
			_mm_storeu_ps(&result.x, sum);
		}
		return result;
	}

	constexpr Quaternion Quaternion::CreateFromRotationMatrix(const Matrix& matrix)
	{
		Float m00 = matrix.m[0][0];
		Float m01 = matrix.m[0][1];
		Float m02 = matrix.m[0][2];
		Float m10 = matrix.m[1][0];
		Float m11 = matrix.m[1][1];
		Float m12 = matrix.m[1][2];
		Float m20 = matrix.m[2][0];
		Float m21 = matrix.m[2][1];
		Float m22 = matrix.m[2][2];

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

	inline constexpr Matrix Matrix::Identity = 
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}