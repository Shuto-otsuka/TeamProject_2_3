#include <GraphicsEngine/Shape/Primitive/PrimitiveMesh.h>

namespace SeedCore
{
	void PrimitiveMesh::Build()
	{
		DynamicArray<Uint32> triangles;
		DynamicArray<Meshlet::TriangleRange> triangleRanges;
		DynamicArray<ShapeKind> rangeKinds;

		DynamicArray<PrimitiveVertex> shapeVertices;
		DynamicArray<Uint32> shapeTriangles;

		auto appendShape = [&](ShapeKind kind)
			{
				Uint32 vertexBase = static_cast<Uint32>(vertices_.size());
				vertices_.insert(vertices_.end(), shapeVertices.begin(), shapeVertices.end());

				Meshlet::TriangleRange triangleRange{};
				triangleRange.triangleOffset_ = static_cast<Uint32>(triangles.size() / 3);
				triangleRange.triangleCount_ = static_cast<Uint32>(shapeTriangles.size() / 3);
				triangleRanges.push_back(triangleRange);
				rangeKinds.push_back(kind);

				for (Uint32 vertexIndex : shapeTriangles)
				{
					triangles.push_back(vertexBase + vertexIndex);
				}

				shapeVertices.clear();
				shapeTriangles.clear();
			};

		CreateBoxShape(shapeVertices, shapeTriangles);
		appendShape(ShapeKind::Box);

		Meshlet meshlet(triangles, triangleRanges);
		meshlets_.assign(meshlet.Meshlets().begin(), meshlet.Meshlets().end());

		vertexIndices_.assign(meshlet.VertexIndices().begin(), meshlet.VertexIndices().end());
	
		primitiveIndices_.assign(meshlet.PrimitiveIndices().begin(), meshlet.PrimitiveIndices().end());

		meshletRanges_.assign(static_cast<Size>(ShapeKind::Arrow) + 1, Meshlet::MeshletRange{});
		for (Size rangeIndex = 0;rangeIndex < rangeKinds.size();++rangeIndex)
		{
			meshletRanges_[static_cast<Size>(rangeKinds[rangeIndex])] = meshlet.MeshletRanges()[rangeIndex];
		}
	}

	std::span<const PrimitiveVertex> PrimitiveMesh::Vertices()const
	{
		return vertices_;
	}

	std::span<const MeshletDesc> PrimitiveMesh::Meshlets()const
	{
		return meshlets_;
	}

	std::span<const Meshlet::MeshletRange> PrimitiveMesh::MeshletRanges()const
	{
		return meshletRanges_;
	}

	std::span<const Uint32> PrimitiveMesh::VertexIndices()const
	{
		return vertexIndices_;
	}

	std::span<const Uint8> PrimitiveMesh::PrimitiveIndices()const
	{
		return primitiveIndices_;
	}

	void PrimitiveMesh::CreateBoxShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles)
	{
		const Vector3 normals[6] =
		{
			{ 1.0f, 0.0f, 0.0f },{ -1.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f },{  0.0f,-1.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f },{  0.0f, 0.0f,-1.0f }
		};
		const Vector3 rights[6] =
		{
			{ 0.0f, 0.0f,-1.0f },{  0.0f, 0.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f },{  1.0f, 0.0f, 0.0f },
			{ 1.0f, 0.0f, 0.0f },{ -1.0f, 0.0f, 0.0f }
		};
		const Vector3 ups[6] =
		{
			{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f,-1.0f },{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f }
		};
		const Vector2 corners[4] =
		{
			{ -1.0f,-1.0f },{  1.0f,-1.0f },
			{  1.0f, 1.0f },{ -1.0f, 1.0f }
		};

		for (Uint32 faceIndex = 0;faceIndex < 6;++faceIndex)
		{
			Uint32 baseIndex = static_cast<Uint32>(vertices.size());
			for (const Vector2& corner : corners)
			{
				PrimitiveVertex vertex{};
				vertex.position_ = normals[faceIndex] + rights[faceIndex] * corner.x + ups[faceIndex] * corner.y;
				vertex.normal_ = normals[faceIndex];
				vertex.uv_ = Vector2(corner.x * 0.5f + 0.5f, 0.5f - corner.y * 0.5f);
				vertex.cap_ = 0.0f;
				vertices.push_back(vertex);
			}
			triangles.insert(triangles.end(), { baseIndex,baseIndex + 1,baseIndex + 2,baseIndex,baseIndex + 2,baseIndex + 3 });
		}
	}
}