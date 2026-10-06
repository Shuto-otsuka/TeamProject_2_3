#include <GraphicsEngine/Model/Cluster/Meshlet.h>

namespace SeedCore
{
	Meshlet::Meshlet(std::span<const Uint32> triangles)
	{
		TriangleRange whole{};
		whole.triangleCount_ = static_cast<Uint32>(triangles.size() / 3);
		Pack(triangles, std::span<const TriangleRange>(&whole, 1));
	}

	Meshlet::Meshlet(std::span<const Uint32> triangles, std::span<const TriangleRange> triangleRanges)
	{
		Pack(triangles, triangleRanges);
	}

	std::span<const MeshletDesc> Meshlet::Meshlets()const
	{
		return meshlets_;
	}

	std::span<const Meshlet::MeshletRange> Meshlet::MeshletRanges()const
	{
		return meshletRanges_;
	}

	std::span<const Uint32> Meshlet::VertexIndices()const
	{
		return vertexIndices_;
	}

	std::span<const Uint8> Meshlet::PrimitiveIndices()const
	{
		return primitiveIndices_;
	}

	void Meshlet::Pack(std::span<const Uint32> triangles, std::span<const TriangleRange> triangleRanges)
	{
		std::unordered_map<Uint32, Uint8> localIndexMap;
		DynamicArray<Uint32> currentVertexIndices;
		DynamicArray<Uint8> currentPrimitiveIndices;

		auto flushMeshlet = [&]()
			{
				if (currentPrimitiveIndices.empty())
				{
					return;
				}

				MeshletDesc meshlet{};
				meshlet.vertexOffset_ = static_cast<Uint32>(vertexIndices_.size());
				meshlet.triangleOffset_ = static_cast<Uint32>(primitiveIndices_.size());
				meshlet.vertexCount_ = static_cast<Uint32>(currentVertexIndices.size());
				meshlet.triangleCount_ = static_cast<Uint32>(currentPrimitiveIndices.size() / 3);
				meshlets_.push_back(meshlet);

				vertexIndices_.insert(vertexIndices_.end(), currentVertexIndices.begin(), currentVertexIndices.end());
				primitiveIndices_.insert(primitiveIndices_.end(), currentPrimitiveIndices.begin(), currentPrimitiveIndices.end());

				localIndexMap.clear();
				currentVertexIndices.clear();
				currentPrimitiveIndices.clear();
			};

		for (const TriangleRange& triangleRange : triangleRanges)
		{
			MeshletRange& meshletRange = meshletRanges_.emplace_back();
			meshletRange.meshletOffset_ = static_cast<Uint32>(meshlets_.size());

			for (Uint32 localTriangle = 0; localTriangle < triangleRange.triangleCount_; ++localTriangle)
			{
				Size base = static_cast<Size>(triangleRange.triangleOffset_ + localTriangle) * 3;
				if (base + 2 >= triangles.size())
				{
					break;
				}

				Uint32 newVertexCount = 0;
				for (Size corner = 0; corner < 3; ++corner)
				{
					if (!localIndexMap.contains(triangles[base + corner]))
					{
						++newVertexCount;
					}
				}

				if (currentVertexIndices.size() + newVertexCount > maxVerticesPerMeshlet_ || currentPrimitiveIndices.size() / 3 + 1 > maxTrianglesPerMeshlet_)
				{
					flushMeshlet();
				}

				for (Size corner = 0; corner < 3; ++corner)
				{
					Uint32 globalIndex = triangles[base + corner];
					auto found = localIndexMap.find(globalIndex);

					Uint8 localIndex = 0;
					if (found == localIndexMap.end())
					{
						localIndex = static_cast<Uint8>(currentVertexIndices.size());
						currentVertexIndices.push_back(globalIndex);
						localIndexMap[globalIndex] = localIndex;
					}
					else
					{
						localIndex = found->second;
					}

					currentPrimitiveIndices.push_back(localIndex);
				}
			}

			flushMeshlet();
			meshletRange.meshletCount_ = static_cast<Uint32>(meshlets_.size() - meshletRange.meshletOffset_);
		}

		Size alignedPrimitiveByteSize = (primitiveIndices_.size() + 3) & ~static_cast<Size>(3);
		primitiveIndices_.resize(alignedPrimitiveByteSize, 0);
	}
}