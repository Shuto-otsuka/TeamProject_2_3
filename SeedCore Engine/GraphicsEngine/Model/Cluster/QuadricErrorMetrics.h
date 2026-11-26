#pragma once

namespace SeedCore
{
	class QuadricErrorMetrics
	{
	public:
		struct Result
		{
			DynamicArray<Uint32> indices;
			DynamicArray<Uint32> vertexMap;
			DynamicArray<Vector3> positions;
		};

		Result Simplify(const Vector3* positions, Size vertexCount, const Uint32* indices, Size indexCount, Size targetTriangleCount, const Bool* locked = nullptr);

	private:
		struct Quadric
		{
			Double d[10] = {};

			void FromPlane(Double a, Double b, Double c, Double w);

			Quadric& operator+=(const Quadric& rhs);

			Double Evaluate(Double x, Double y, Double z)const;

			Bool OptimalPosition(Double& outX, Double& outY, Double& outZ)const;
		};
	};
}