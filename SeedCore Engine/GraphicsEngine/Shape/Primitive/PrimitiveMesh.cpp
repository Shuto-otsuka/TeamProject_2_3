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

		/// [EN] One culling bound per meshlet in unit-mesh space: the sphere is centered on the average vertex position, and the normal cone is built from the vertex normals, which are exact for the unit meshes.
		/// [JP] 単位メッシュの空間で、メッシュレットごとにカリング用の範囲を作る。球の中心は頂点の位置の平均。法線コーンは、単位メッシュでは正確な頂点の法線から作る。
		meshletBounds_.resize(meshlets_.size());
		for (Size meshletIndex = 0; meshletIndex < meshlets_.size(); ++meshletIndex)
		{
			const MeshletDesc& meshletDesc = meshlets_[meshletIndex];
			MeshletBound& bound = meshletBounds_[meshletIndex];

			Vector3 center = Vector3::Zero;
			Vector3 axis = Vector3::Zero;
			for (Uint32 vertexIndex = 0; vertexIndex < meshletDesc.vertexCount_; ++vertexIndex)
			{
				const PrimitiveVertex& vertex = vertices_[vertexIndices_[meshletDesc.vertexOffset_ + vertexIndex]];
				center += vertex.position_;
				axis += vertex.normal_;
			}
			center /= static_cast<Float>(meshletDesc.vertexCount_);

			Float radius = 0.0f;
			for (Uint32 vertexIndex = 0; vertexIndex < meshletDesc.vertexCount_; ++vertexIndex)
			{
				radius = Max(radius, (vertices_[vertexIndices_[meshletDesc.vertexOffset_ + vertexIndex]].position_ - center).Length());
			}
			bound.center_ = center;
			bound.radius_ = radius;

			/// [EN] The cone cutoff is the smallest cosine between any vertex normal and the average axis; when the normals cancel out (a closed shape in one meshlet) there is no useful cone, and 0 turns the cone test off.
			/// [JP] コーンの境界は、各頂点の法線と平均の軸のなす角の cos の最小値。法線が打ち消し合う（閉じた形が 1 つのメッシュレットに入っている）ときは使えるコーンが無いので、0 にしてコーンの判定を使わない。
			Float axisLength = axis.Length();
			if (axisLength > 1e-6f)
			{
				axis /= axisLength;
				Float minDot = 1.0f;
				for (Uint32 vertexIndex = 0; vertexIndex < meshletDesc.vertexCount_; ++vertexIndex)
				{
					minDot = Min(minDot, vertices_[vertexIndices_[meshletDesc.vertexOffset_ + vertexIndex]].normal_.Dot(axis));
				}
				bound.coneAxis_ = axis;
				bound.coneCutoff_ = minDot;
			}
			else
			{
				bound.coneAxis_ = Vector3(0.0f, 0.0f, 1.0f);
				bound.coneCutoff_ = 0.0f;
			}
		}

		for (const Meshlet::MeshletRange& meshletRange : meshletRanges_)
		{
			SC_ASSERT(meshletRange.meshletCount_ <= maxMeshletsPerShape_);
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

	std::span<const MeshletBound> PrimitiveMesh::MeshletBounds()const
	{
		return meshletBounds_;
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
				vertex.texcoord_ = Vector2(corner.x * 0.5f + 0.5f, 0.5f - corner.y * 0.5f);
				vertex.cap_ = 0.0f;
				vertices.push_back(vertex);
			}
			triangles.insert(triangles.end(), { baseIndex + 2,baseIndex + 1,baseIndex,baseIndex + 3,baseIndex + 2,baseIndex });
		}
	}
}