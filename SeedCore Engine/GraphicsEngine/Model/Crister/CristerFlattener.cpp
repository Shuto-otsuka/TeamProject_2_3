#include <GraphicsEngine/Model/Crister/CristerFlattener.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	void CristerFlattener::Flatten(Crister& crister, std::span<const DynamicArray<Matrix>> subMeshPlacements)
	{
		triangleIndices_.clear();
		vertices_.clear();
		positions_.clear();
		skinVertices_.clear();
		morphDeltas_.clear();

		CristerQuantizer quantizer;
		Bool degeneratePositionLogged = false;

		skinVerticesBuilt_ = crister.scene_.skins_.size() == 1 && !crister.geometry_.compressedSkinVertices_.empty();
		std::unordered_map<Uint32, Uint32> raytracingRemap;
		for (Size subMeshIndex = 0; subMeshIndex < crister.geometry_.subMeshes_.size(); subMeshIndex++)
		{
			SubMesh& subMesh = crister.geometry_.subMeshes_[subMeshIndex];

			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			const Cluster& cluster = crister.geometry_.clusters_[subMesh.clusterOffset_];

			subMesh.raytracingVertexOffset_ = static_cast<Uint32>(vertices_.size());
			subMesh.raytracingTriangleOffset_ = static_cast<Uint32>(triangleIndices_.size() / 3);
			DynamicArray<DynamicArray<Vector3>> submeshMorphDeltas(subMesh.morphs_.size());

			const DynamicArray<Matrix>& placements = subMeshPlacements[subMeshIndex];

			Size placementCount = subMesh.morphs_.empty() ? placements.size() : 1;
			for (Size placementIndex = 0; placementIndex < placementCount; placementIndex++)
			{
				const Matrix& placement = placements[placementIndex];
				Bool placed = placement != Matrix::Identity;
				Matrix normalPlacement = placement.Invert().Transpose();
				Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

				raytracingRemap.clear();

				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];
							auto found = raytracingRemap.find(globalIndex);
							if (found == raytracingRemap.end())
							{
								Uint32 compactIndex = static_cast<Uint32>(vertices_.size());
								raytracingRemap[globalIndex] = compactIndex;

								const CompressedVertex& compressed = crister.geometry_.compressedVertices_[globalIndex];

								Vector3 position = quantizer.DecodePosition(crister, compressed);
								if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
								{
									if (!degeneratePositionLogged)
									{
										SC_LOG_WARNING("RT プロキシの頂点位置が非有限です。ソースアセットの量子化 AABB が壊れている可能性があります。");
										degeneratePositionLogged = true;
									}
									position = Vector3::Zero;
								}

								if (placed)
								{
									Vertex placedVertex = quantizer.DecodeVertex(crister, compressed);
									placedVertex.position_ = Vector3::TransformPosition(position, placement);
									placedVertex.normal_ = Vector3::TransformDirection(placedVertex.normal_, normalPlacement);
									placedVertex.normal_.Normalize();
									Vector3 placedTangent = Vector3::TransformDirection(placedVertex.tangent_.xyz, placement);
									placedTangent.Normalize();
									placedVertex.tangent_ = Vector4(placedTangent, placedVertex.tangent_.w * tangentSign);
									position = placedVertex.position_;
									vertices_.push_back(quantizer.EncodeVertex(placedVertex, crister.geometry_.positionMin_, crister.geometry_.positionExtent_, crister.geometry_.texcoordMin_, crister.geometry_.texcoordExtent_));
								}
								else
								{
									vertices_.push_back(compressed);
								}

								positions_.push_back(position);
								triangleIndices_.push_back(compactIndex);

								if (skinVerticesBuilt_)
								{
									if (subMesh.skinIndex_ >= 0 && globalIndex < crister.geometry_.compressedSkinVertices_.size())
									{
										skinVertices_.push_back(crister.geometry_.compressedSkinVertices_[globalIndex]);
									}
									else
									{
										skinVertices_.push_back(CompressedSkinVertex{});
									}
								}

								if (!subMesh.morphs_.empty() && globalIndex >= subMesh.vertexOffset_)
								{
									Uint32 localVertexIndex = globalIndex - subMesh.vertexOffset_;
									for (Size targetIndex = 0; targetIndex < subMesh.morphs_.size(); targetIndex++)
									{
										const DynamicArray<Vector3>& deltas = subMesh.morphs_[targetIndex].positionDeltas_;
										Vector3 delta = localVertexIndex < deltas.size() ? deltas[localVertexIndex] : Vector3::Zero;
										submeshMorphDeltas[targetIndex].push_back(placed ? Vector3::TransformDirection(delta, placement) : delta);
									}
								}
							}
							else
							{
								triangleIndices_.push_back(found->second);
							}
						}
					}
				}
			}

			subMesh.raytracingVertexCount_ = static_cast<Uint32>(vertices_.size()) - subMesh.raytracingVertexOffset_;
			subMesh.raytracingTriangleCount_ = static_cast<Uint32>(triangleIndices_.size() / 3) - subMesh.raytracingTriangleOffset_;

			if (!subMesh.morphs_.empty())
			{
				subMesh.raytracingMorphDeltaOffset_ = static_cast<Uint32>(morphDeltas_.size());
				for (const DynamicArray<Vector3>& targetDeltas : submeshMorphDeltas)
				{
					morphDeltas_.insert(morphDeltas_.end(), targetDeltas.begin(), targetDeltas.end());
				}
			}
		}

		placedPositionMin_ = crister.geometry_.positionMin_;
		placedPositionExtent_ = crister.geometry_.positionExtent_;
		if (!positions_.empty())
		{
			Vector3 placedMax = positions_.front();
			placedPositionMin_ = positions_.front();
			for (const Vector3& position : positions_)
			{
				placedPositionMin_ = Vector3::Min(placedPositionMin_, position);
				placedMax = Vector3::Max(placedMax, position);
			}
			placedPositionExtent_ = placedMax - placedPositionMin_;
		}
	}

	std::span<const CompressedVertex> CristerFlattener::Vertices()const
	{
		return vertices_;
	}

	std::span<const Vector3> CristerFlattener::Positions()const
	{
		return positions_;
	}

	std::span<const Uint32> CristerFlattener::TriangleIndices()const
	{
		return triangleIndices_;
	}

	std::span<const CompressedSkinVertex> CristerFlattener::SkinVertices()const
	{
		return skinVertices_;
	}

	std::span<const Vector3> CristerFlattener::MorphDeltas()const
	{
		return morphDeltas_;
	}

	Bool CristerFlattener::SkinVerticesBuilt()const
	{
		return skinVerticesBuilt_;
	}

	Vector3 CristerFlattener::PlacedPositionMin()const
	{
		return placedPositionMin_;
	}

	Vector3 CristerFlattener::PlacedPositionExtent()const
	{
		return placedPositionExtent_;
	}
}