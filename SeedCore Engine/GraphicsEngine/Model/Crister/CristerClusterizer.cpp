#include <GraphicsEngine/Model/Crister/CristerClusterizer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Cluster/QuadricErrorMetrics.h>

namespace SeedCore
{
	void CristerClusterizer::BuildMeshlets(Crister& crister)const
	{
		constexpr Uint32 maxLodLevels = 24;

		DynamicArray<Uint32> originalIndices = std::move(crister.geometry_.vertexIndices_);
		crister.geometry_.vertexIndices_.clear();
		crister.geometry_.primitiveIndices_.clear();
		crister.geometry_.meshlets_.clear();
		crister.geometry_.meshletBounds_.clear();
		crister.geometry_.clusters_.clear();

		auto buildMeshletsFromIndices = [&](const Uint32* indices, Size indexCount) -> void
			{
				Meshlet meshlet(std::span<const Uint32>(indices, indexCount));
				std::span<const MeshletDesc> meshlets = meshlet.Meshlets();

				Uint32 vertexBase = static_cast<Uint32>(crister.geometry_.vertexIndices_.size());
				Uint32 triangleBase = static_cast<Uint32>(crister.geometry_.primitiveIndices_.size());
				for (const MeshletDesc& source : meshlets)
				{
					MeshletDesc& destination = crister.geometry_.meshlets_.emplace_back(source);
					destination.vertexOffset_ += vertexBase;
					destination.triangleOffset_ += triangleBase;
				}

				std::span<const Uint32> vertexIndices = meshlet.VertexIndices();
				crister.geometry_.vertexIndices_.insert(crister.geometry_.vertexIndices_.end(), vertexIndices.begin(), vertexIndices.end());

				Size usedByteCount = meshlets.empty() ? 0 : static_cast<Size>(meshlets.back().triangleOffset_) + static_cast<Size>(meshlets.back().triangleCount_) * 3;
				std::span<const Uint8> primitiveIndices = meshlet.PrimitiveIndices().first(usedByteCount);
				crister.geometry_.primitiveIndices_.insert(crister.geometry_.primitiveIndices_.end(), primitiveIndices.begin(), primitiveIndices.end());
			};

		auto computeBounds = [&](Uint32 meshletBegin, Uint32 meshletEnd)
			{
				for (Uint32 meshletIndex = meshletBegin; meshletIndex < meshletEnd; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					MeshletBound& bound = crister.geometry_.meshletBounds_.emplace_back();

					Vector3 center = Vector3::Zero;
					for (Uint32 index = 0; index < meshlet.vertexCount_; index++)
					{
						Uint32 vertexIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + index];
						center += crister.geometry_.vertices_[vertexIndex].position_;
					}
					center /= static_cast<Float>(meshlet.vertexCount_);

					Float maxDistance2 = 0;
					for (Uint32 index = 0; index < meshlet.vertexCount_; index++)
					{
						Uint32 vertexIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + index];
						Vector3 difference = crister.geometry_.vertices_[vertexIndex].position_ - center;
						Float distance2 = difference.Dot(difference);
						if (distance2 > maxDistance2)
						{
							maxDistance2 = distance2;
						}
					}

					bound.center_ = center;
					bound.radius_ = std::sqrt(maxDistance2);

					Vector3 avgNormal = Vector3::Zero;
					Uint32 faceCount = 0;

					for (Uint32 triangle = 0; triangle < meshlet.triangleCount_; triangle++)
					{
						Uint32 base = meshlet.triangleOffset_ + triangle * 3;

						Uint32 gi0 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base]];
						Uint32 gi1 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 1]];
						Uint32 gi2 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 2]];

						Vector3 p0 = crister.geometry_.vertices_[gi0].position_;
						Vector3 p1 = crister.geometry_.vertices_[gi1].position_;
						Vector3 p2 = crister.geometry_.vertices_[gi2].position_;

						Vector3 e1 = p1 - p0;
						Vector3 e2 = p2 - p0;
						Vector3 n = e2.Cross(e1);
						Float length = n.Length();
						if (length < 1e-8f)
						{
							continue;
						}
						n /= length;

						avgNormal += n;
						faceCount++;
					}

					if (faceCount > 0)
					{
						Float length = avgNormal.Length();
						if (length > 1e-8f)
						{
							avgNormal /= length;
							bound.coneAxis_ = avgNormal;

							Float minDot = 1.0f;
							for (Uint32 triangle = 0; triangle < meshlet.triangleCount_; triangle++)
							{
								Uint32 base = meshlet.triangleOffset_ + triangle * 3;
								Uint32 gi0 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base]];
								Uint32 gi1 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 1]];
								Uint32 gi2 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 2]];

								Vector3 p0 = crister.geometry_.vertices_[gi0].position_;
								Vector3 p1 = crister.geometry_.vertices_[gi1].position_;
								Vector3 p2 = crister.geometry_.vertices_[gi2].position_;

								Vector3 e1 = p1 - p0;
								Vector3 e2 = p2 - p0;
								Vector3 n = e2.Cross(e1);
								Float length2 = n.Length();
								if (length2 < 1e-8f)
								{
									continue;
								}
								n /= length2;

								Float dot = n.Dot(avgNormal);
								if (dot < minDot)
								{
									minDot = dot;
								}
							}
							bound.coneCutoff_ = minDot;
						}
					}
				}
			};

		auto computeBoundaryLocks = [](const Uint32* indices, Size indexCount, Size vertexCount) -> std::unique_ptr<Bool[]>
			{
				std::unordered_map<Uint64, Uint32> edgeUseCount;
				Size triangleCount = indexCount / 3;
				for (Size triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
				{
					Uint32 triangleVertices[3] = { indices[triangleIndex * 3], indices[triangleIndex * 3 + 1], indices[triangleIndex * 3 + 2] };
					for (Int edgeIndex = 0; edgeIndex < 3; edgeIndex++)
					{
						Uint32 vertexA = triangleVertices[edgeIndex];
						Uint32 vertexB = triangleVertices[(edgeIndex + 1) % 3];
						if (vertexA > vertexB)
						{
							std::swap(vertexA, vertexB);
						}
						Uint64 key = (static_cast<Uint64>(vertexA) << 32) | vertexB;
						edgeUseCount[key]++;
					}
				}

				std::unique_ptr<Bool[]> locked = std::make_unique<Bool[]>(vertexCount);
				for (const auto& [key, count] : edgeUseCount)
				{
					if (count == 1)
					{
						locked[static_cast<Uint32>(key >> 32)] = true;
						locked[static_cast<Uint32>(key & 0xFFFFFFFF)] = true;
					}
				}
				return locked;
			};

		for (SubMesh& sub : crister.geometry_.subMeshes_)
		{
			sub.clusterOffset_ = static_cast<Uint32>(crister.geometry_.clusters_.size());

			DynamicArray<Uint32> currentIndices(originalIndices.begin() + sub.indexOffset_, originalIndices.begin() + sub.indexOffset_ + sub.indexCount_);

			{
				Double worldAreaSum = 0.0;
				Double texcoordAreaSum = 0.0;
				for (Size corner = 0; corner + 2 < currentIndices.size(); corner += 3)
				{
					const Vertex& vertex0 = crister.geometry_.vertices_[currentIndices[corner + 0]];
					const Vertex& vertex1 = crister.geometry_.vertices_[currentIndices[corner + 1]];
					const Vertex& vertex2 = crister.geometry_.vertices_[currentIndices[corner + 2]];

					Vector3 edge1 = vertex1.position_ - vertex0.position_;
					Vector3 edge2 = vertex2.position_ - vertex0.position_;
					worldAreaSum += 0.5 * static_cast<Double>(edge1.Cross(edge2).Length());

					Vector2 texcoordEdge1 = vertex1.texcoord_ - vertex0.texcoord_;
					Vector2 texcoordEdge2 = vertex2.texcoord_ - vertex0.texcoord_;
					texcoordAreaSum += 0.5 * std::abs(static_cast<Double>(texcoordEdge1.x) * texcoordEdge2.y - static_cast<Double>(texcoordEdge2.x) * texcoordEdge1.y);
				}
				sub.texcoordDensity_ = (worldAreaSum > 1e-12 && texcoordAreaSum > 1e-12) ? static_cast<Float>(std::sqrt(texcoordAreaSum / worldAreaSum)) : 0.0f;
			}

			std::set<Uint32> usedSet(currentIndices.begin(), currentIndices.end());
			DynamicArray<Uint32> localToGlobal(usedSet.begin(), usedSet.end());
			std::unordered_map<Uint32, Uint32> globalToLocal;
			for (Uint32 index = 0; index < static_cast<Uint32>(localToGlobal.size()); index++)
			{
				globalToLocal[localToGlobal[index]] = index;
			}

			DynamicArray<Vector3> localPositions(localToGlobal.size());
			for (Size index = 0; index < localToGlobal.size(); index++)
			{
				localPositions[index] = crister.geometry_.vertices_[localToGlobal[index]].position_;
			}

			DynamicArray<Uint32> localIndices(currentIndices.size());
			for (Size index = 0; index < currentIndices.size(); index++)
			{
				localIndices[index] = globalToLocal[currentIndices[index]];
			}

			Uint32 meshletBegin = static_cast<Uint32>(crister.geometry_.meshlets_.size());
			buildMeshletsFromIndices(currentIndices.data(), currentIndices.size());
			Uint32 meshletEnd = static_cast<Uint32>(crister.geometry_.meshlets_.size());
			computeBounds(meshletBegin, meshletEnd);

			Cluster& lod0 = crister.geometry_.clusters_.emplace_back();
			lod0.meshletOffset_ = meshletBegin;
			lod0.meshletCount_ = meshletEnd - meshletBegin;
			lod0.lodLevel_ = 0;
			lod0.lodError_ = 0.0f;

			DynamicArray<Vector3> lodPositions = localPositions;
			DynamicArray<Uint32> lodIndices = localIndices;

			DynamicArray<Uint32> lodVertexMap(localToGlobal.size());
			for (Uint32 index = 0; index < static_cast<Uint32>(localToGlobal.size()); index++)
			{
				lodVertexMap[index] = localToGlobal[index];
			}

			Float accumulatedError = 0.0f;

			for (Uint32 lod = 1; lod < maxLodLevels; lod++)
			{
				Size currentTriangleCount = lodIndices.size() / 3;

				Size targetTriangleCount = currentTriangleCount / 2;
				if (targetTriangleCount < 4)
				{
					break;
				}

				std::unique_ptr<Bool[]> boundaryLocks = computeBoundaryLocks(lodIndices.data(), lodIndices.size(), lodPositions.size());
				QuadricErrorMetrics qem;
				auto result = qem.Simplify(lodPositions.data(), lodPositions.size(), lodIndices.data(), lodIndices.size(), targetTriangleCount, boundaryLocks.get());

				if (result.indices.size() / 3 >= currentTriangleCount)
				{
					break;
				}

				Uint32 newVertexBase = static_cast<Uint32>(crister.geometry_.vertices_.size());
				crister.geometry_.vertices_.resize(newVertexBase + result.positions.size());
				crister.geometry_.vertexMorphSource_.resize(newVertexBase + result.positions.size());

				DynamicArray<Uint32> newLodVertexMap(result.vertexMap.size());
				for (Size index = 0; index < result.positions.size(); index++)
				{
					Uint32 originalLocal = result.vertexMap[index];
					Uint32 globalIndex = lodVertexMap[originalLocal];

					Vertex vertex = crister.geometry_.vertices_[globalIndex];
					vertex.position_ = result.positions[index];
					crister.geometry_.vertices_[newVertexBase + index] = vertex;

					crister.geometry_.vertexMorphSource_[newVertexBase + index] = crister.geometry_.vertexMorphSource_[globalIndex];
					newLodVertexMap[index] = newVertexBase + static_cast<Uint32>(index);
				}

				DynamicArray<Uint32> globalLodIndices(result.indices.size());
				for (Size index = 0; index < result.indices.size(); index++)
				{
					globalLodIndices[index] = newVertexBase + result.indices[index];
				}

				Uint32 lodMeshletBegin = static_cast<Uint32>(crister.geometry_.meshlets_.size());
				buildMeshletsFromIndices(globalLodIndices.data(), globalLodIndices.size());
				Uint32 lodMeshletEnd = static_cast<Uint32>(crister.geometry_.meshlets_.size());
				computeBounds(lodMeshletBegin, lodMeshletEnd);

				Float maxLevelError = 0.0f;
				for (Size index = 0; index < result.positions.size(); index++)
				{
					Uint32 originalLocal = result.vertexMap[index];
					Vector3 difference = result.positions[index] - lodPositions[originalLocal];
					Float error = difference.Length();
					if (error > maxLevelError)
					{
						maxLevelError = error;
					}
				}
				accumulatedError += maxLevelError;

				Cluster& lodCluster = crister.geometry_.clusters_.emplace_back();
				lodCluster.meshletOffset_ = lodMeshletBegin;
				lodCluster.meshletCount_ = lodMeshletEnd - lodMeshletBegin;
				lodCluster.lodLevel_ = lod;
				lodCluster.lodError_ = accumulatedError;

				lodPositions = std::move(result.positions);
				lodIndices = std::move(result.indices);
				lodVertexMap = std::move(newLodVertexMap);
			}

			sub.clusterCount_ = static_cast<Uint32>(crister.geometry_.clusters_.size()) - sub.clusterOffset_;
		}
	}
}