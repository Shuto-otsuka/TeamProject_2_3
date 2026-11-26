#include <GraphicsEngine/Model/Crister/CristerExtractor.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
	Bool CristerExtractor::Collision(const Crister& crister, MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const
	{
		outPositions.clear();
		outIndices.clear();

		if (crister.geometry_.compressedVertices_.empty() || crister.geometry_.subMeshes_.empty())
		{
			return false;
		}

		CristerQuantizer quantizer;

		std::unordered_map<Uint32, Uint32> remap;

		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			Uint32 clusterIndex = detail == MeshCollisionDetail::Exact ? subMesh.clusterOffset_ : subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;

			if (clusterIndex >= crister.geometry_.clusters_.size())
			{
				continue;
			}

			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];

			DynamicArray<Matrix> placements;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				for (const Node& node : crister.scene_.nodes_)
				{
					if (node.mesh_ == subMesh.meshIndex_)
					{
						placements.push_back(node.globalTransform_);
					}
				}
			}
			if (placements.empty())
			{
				placements.push_back(Matrix::Identity);
			}

			for (const Matrix& placement : placements)
			{
				remap.clear();

				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];
							auto found = remap.find(globalIndex);
							if (found == remap.end())
							{
								Uint32 compactIndex = static_cast<Uint32>(outPositions.size());
								remap[globalIndex] = compactIndex;
								outPositions.push_back(Vector3::TransformPosition(quantizer.DecodePosition(crister, crister.geometry_.compressedVertices_[globalIndex]), placement));
								outIndices.push_back(compactIndex);
							}
							else
							{
								outIndices.push_back(found->second);
							}
						}
					}
				}
			}
		}

		return outIndices.size() >= 3;
	}

	Bool CristerExtractor::FinestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		outVertices.clear();
		outIndices.clear();

		if (crister.geometry_.compressedVertices_.empty() || crister.geometry_.subMeshes_.empty())
		{
			return false;
		}

		CristerQuantizer quantizer;

		outVertices.reserve(crister.geometry_.compressedVertices_.size());
		for (const CompressedVertex& compressed : crister.geometry_.compressedVertices_)
		{
			outVertices.push_back(quantizer.DecodeVertex(crister, compressed));
		}

		DynamicArray<Bool> placedVertices(outVertices.size(), false);
		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			Uint32 clusterIndex = subMesh.clusterOffset_;
			if (clusterIndex >= crister.geometry_.clusters_.size())
			{
				continue;
			}

			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(crister.scene_.nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != crister.scene_.nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];

						outIndices.push_back(globalIndex);

						if (placed && !placedVertices[globalIndex])
						{
							placedVertices[globalIndex] = true;
							Vertex& vertex = outVertices[globalIndex];
							vertex.position_ = Vector3::TransformPosition(vertex.position_, placement);
							vertex.normal_ = Vector3::TransformDirection(vertex.normal_, normalPlacement);
							vertex.normal_.Normalize();
							Vector3 tangent = Vector3::TransformDirection(vertex.tangent_.xyz, placement);
							tangent.Normalize();
							vertex.tangent_ = Vector4(tangent, vertex.tangent_.w * tangentSign);
						}
					}
				}
			}
		}

		return outIndices.size() >= 3;
	}

	Bool CristerExtractor::CoarsestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		outVertices.clear();
		outIndices.clear();

		if (crister.geometry_.compressedVertices_.empty() || crister.geometry_.subMeshes_.empty())
		{
			return false;
		}

		CristerQuantizer quantizer;

		auto quantisedPositionKey = [](const Vector3& position) -> Uint64
		{
			constexpr Float gridScale = 10000.0f;
			Int64 x = static_cast<Int64>(std::lround(position.x * gridScale));
			Int64 y = static_cast<Int64>(std::lround(position.y * gridScale));
			Int64 z = static_cast<Int64>(std::lround(position.z * gridScale));

			Uint64 hash = static_cast<Uint64>(x) * 73856093ull;
			hash ^= static_cast<Uint64>(y) * 19349663ull;
			hash ^= static_cast<Uint64>(z) * 83492791ull;
			return hash;
		};

		std::unordered_map<Uint64, Uint32> remap;

		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			Uint32 clusterIndex = subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;
			if (clusterIndex >= crister.geometry_.clusters_.size())
			{
				continue;
			}

			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(crister.scene_.nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != crister.scene_.nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];
						const CompressedVertex& compressed = crister.geometry_.compressedVertices_[globalIndex];

						Vertex vertex;

						vertex.position_ = quantizer.DecodePosition(crister, compressed);
						if (placed)
						{
							vertex.position_ = Vector3::TransformPosition(vertex.position_, placement);
						}

						Uint64 key = quantisedPositionKey(vertex.position_);
						auto found = remap.find(key);
						if (found == remap.end())
						{
							Uint32 compactIndex = static_cast<Uint32>(outVertices.size());
							remap[key] = compactIndex;

							vertex.normal_ = quantizer.DecodeOctahedralNormal(compressed.normal_);
							vertex.tangent_ = quantizer.DecodeTangent(compressed);
							vertex.texcoord_ = quantizer.DecodeTexcoord(crister, compressed);
							if (placed)
							{
								vertex.normal_ = Vector3::TransformDirection(vertex.normal_, normalPlacement);
								vertex.normal_.Normalize();
								Vector3 tangent = Vector3::TransformDirection(vertex.tangent_.xyz, placement);
								tangent.Normalize();
								vertex.tangent_ = Vector4(tangent, vertex.tangent_.w * tangentSign);
							}
							outVertices.push_back(vertex);

							outIndices.push_back(compactIndex);
						}
						else
						{
							outIndices.push_back(found->second);
						}
					}
				}
			}
		}

		return outIndices.size() >= 3;
	}
}
