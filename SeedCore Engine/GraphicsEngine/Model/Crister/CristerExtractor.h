#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	class SEEDCORE_API CristerExtractor
	{
	public:
		NODISCARD Bool Collision(const Crister& crister, MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const;

		NODISCARD Bool FinestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const;

		NODISCARD Bool CoarsestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const;
	};
}
