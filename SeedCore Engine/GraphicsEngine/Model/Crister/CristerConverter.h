#pragma once
#include <FoundationEngine/Resource/Asset/AxisConvention.h>

namespace SeedCore
{
	class Crister;

	class SEEDCORE_API CristerConverter
	{
	public:
		Bool ApplyAxisConversion(Crister& crister, const Matrix& deltaBasis, Bool flipWinding, const std::filesystem::path& cristerPath)const;

		Bool ApplyTransformConversion(Crister& crister, Vector3 position, Vector3 rotation, Vector3 scale, Vector3 pivot, const std::filesystem::path& cristerPath)const;

		void ConvertAxisConvention(Crister& crister, const AxisConvention& axisConvention)const;

	private:
		void ConvertPositionByBasis(Vector3& vector, const Matrix& basis)const;

		void ConvertPositionByBasis(Vector4& vector, const Matrix& basis)const;

		void ConvertRotationByBasis(Quaternion& quaternion, const Matrix& basis)const;

		void ConvertMatrixByBasis(Matrix& matrix, const Matrix& basis)const;

		void FlipTriangleWinding(Crister& crister)const;
	};
}