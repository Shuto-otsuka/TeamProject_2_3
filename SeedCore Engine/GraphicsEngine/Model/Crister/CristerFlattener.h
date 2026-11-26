#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	class SEEDCORE_API CristerFlattener
	{
	public:
		void Flatten(Crister& crister, std::span<const DynamicArray<Matrix>> subMeshPlacements);

		NODISCARD std::span<const CompressedVertex> Vertices()const;

		NODISCARD std::span<const Vector3> Positions()const;

		NODISCARD std::span<const Uint32> TriangleIndices()const;

		NODISCARD std::span<const CompressedSkinVertex> SkinVertices()const;

		NODISCARD std::span<const Vector3> MorphDeltas()const;

		NODISCARD Bool SkinVerticesBuilt()const;

		NODISCARD Vector3 PlacedPositionMin()const;

		NODISCARD Vector3 PlacedPositionExtent()const;

	private:
		DynamicArray<CompressedVertex> vertices_;

		DynamicArray<Vector3> positions_;

		DynamicArray<Uint32> triangleIndices_;

		DynamicArray<CompressedSkinVertex> skinVertices_;

		DynamicArray<Vector3> morphDeltas_;

		Bool skinVerticesBuilt_ = false;

		Vector3 placedPositionMin_ = { 0.0f, 0.0f, 0.0f };

		Vector3 placedPositionExtent_ = { 1.0f, 1.0f, 1.0f };
	};
}