#include <GraphicsEngine/Model/Cluster/QuadricErrorMetrics.h>

namespace SeedCore
{
	void QuadricErrorMetrics::Quadric::FromPlane(Double a, Double b, Double c, Double w)
	{
		d[0] = a * a;
		d[1] = a * b;
		d[2] = a * c;
		d[3] = a * w;
		d[4] = b * b;
		d[5] = b * c;
		d[6] = b * w;
		d[7] = c * c;
		d[8] = c * w;
		d[9] = w * w;
	}

	QuadricErrorMetrics::Quadric& QuadricErrorMetrics::Quadric::operator+=(const Quadric& rhs)
	{
		for (Int index = 0; index < 10; index++)
		{
			d[index] += rhs.d[index];
		}
		return *this;
	}

	Double QuadricErrorMetrics::Quadric::Evaluate(Double x, Double y, Double z)const
	{
		return d[0] * x * x + 2.0 * d[1] * x * y + 2.0 * d[2] * x * z + 2.0 * d[3] * x
			+ d[4] * y * y + 2.0 * d[5] * y * z + 2.0 * d[6] * y
			+ d[7] * z * z + 2.0 * d[8] * z
			+ d[9];
	}

	Bool QuadricErrorMetrics::Quadric::OptimalPosition(Double& outX, Double& outY, Double& outZ)const
	{
		Double a = d[0];
		Double b = d[1];
		Double c = d[2];
		Double e = d[4];
		Double f = d[5];
		Double h = d[7];

		Double determinant = a * (e * h - f * f) - b * (b * h - f * c) + c * (b * f - e * c);

		if (std::abs(determinant) < 1e-15)
		{
			return false;
		}

		Double inverseDeterminant = 1.0 / determinant;

		Double rightX = -d[3];
		Double rightY = -d[6];
		Double rightZ = -d[8];

		outX = inverseDeterminant * ((e * h - f * f) * rightX + (f * c - b * h) * rightY + (b * f - e * c) * rightZ);
		outY = inverseDeterminant * ((f * c - b * h) * rightX + (a * h - c * c) * rightY + (b * c - a * f) * rightZ);
		outZ = inverseDeterminant * ((b * f - e * c) * rightX + (b * c - a * f) * rightY + (a * e - b * b) * rightZ);

		return true;
	}

	QuadricErrorMetrics::Result QuadricErrorMetrics::Simplify(const Vector3* sourcePositions, Size vertexCount, const Uint32* sourceIndices, Size indexCount, Size targetTriangleCount, const Bool* locked)
	{
		Size triangleCount = indexCount / 3;

		if (targetTriangleCount >= triangleCount)
		{
			Result result;
			result.positions.assign(sourcePositions, sourcePositions + vertexCount);
			result.indices.assign(sourceIndices, sourceIndices + indexCount);
			result.vertexMap.resize(vertexCount);
			for (Size index = 0; index < vertexCount; index++)
			{
				result.vertexMap[index] = static_cast<Uint32>(index);
			}
			return result;
		}

		DynamicArray<Vector3> positions(sourcePositions, sourcePositions + vertexCount);
		DynamicArray<Uint32> indices(sourceIndices, sourceIndices + indexCount);

		DynamicArray<Quadric> quadrics(vertexCount);

		DynamicArray<DynamicArray<Uint32>> vertexTriangles(vertexCount);

		DynamicArray<Bool> triangleDead(triangleCount, false);
		DynamicArray<Bool> vertexDead(vertexCount, false);

		Size degenerateTriangleCount = 0;
		for (Size triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
		{
			Uint32 index0 = indices[triangleIndex * 3];
			Uint32 index1 = indices[triangleIndex * 3 + 1];
			Uint32 index2 = indices[triangleIndex * 3 + 2];

			Vector3 edge1 = positions[index1] - positions[index0];
			Vector3 edge2 = positions[index2] - positions[index0];

			Vector3 normal = edge1.Cross(edge2);
			Float length = normal.Length();
			if (length < 1e-8f)
			{
				triangleDead[triangleIndex] = true;
				degenerateTriangleCount++;
				continue;
			}
			normal /= length;

			Double normalX = normal.x;
			Double normalY = normal.y;
			Double normalZ = normal.z;
			Double planeW = -(normalX * positions[index0].x + normalY * positions[index0].y + normalZ * positions[index0].z);

			Quadric quadric;
			quadric.FromPlane(normalX, normalY, normalZ, planeW);
			quadrics[index0] += quadric;
			quadrics[index1] += quadric;
			quadrics[index2] += quadric;

			vertexTriangles[index0].push_back(static_cast<Uint32>(triangleIndex));
			vertexTriangles[index1].push_back(static_cast<Uint32>(triangleIndex));
			vertexTriangles[index2].push_back(static_cast<Uint32>(triangleIndex));
		}

		struct EdgeEntry
		{
			Uint32 vertex0;
			Uint32 vertex1;
			Double cost;
			Vector3 optimalPosition;
			Uint32 version;
			Bool operator>(const EdgeEntry& other)const { return cost > other.cost; }
		};

		auto makeEdgeKey = [](Uint32 vertexA, Uint32 vertexB) -> Uint64
			{
				if (vertexA > vertexB)
				{
					std::swap(vertexA, vertexB);
				}
				return (static_cast<Uint64>(vertexA) << 32) | vertexB;
			};

		std::unordered_map<Uint64, Uint32> edgeVersions;

		auto calculateEdgeCost = [&](Uint32 vertex0, Uint32 vertex1) -> EdgeEntry
			{
				EdgeEntry entry;
				entry.vertex0 = vertex0;
				entry.vertex1 = vertex1;

				Quadric combinedQuadric = quadrics[vertex0];
				combinedQuadric += quadrics[vertex1];

				Bool vertex0Locked = locked && locked[vertex0];
				Bool vertex1Locked = locked && locked[vertex1];

				Double optimalX, optimalY, optimalZ;
				if (!vertex0Locked && !vertex1Locked && combinedQuadric.OptimalPosition(optimalX, optimalY, optimalZ))
				{
					entry.optimalPosition = Vector3(static_cast<Float>(optimalX), static_cast<Float>(optimalY), static_cast<Float>(optimalZ));
					entry.cost = combinedQuadric.Evaluate(optimalX, optimalY, optimalZ);
				}
				else if (vertex0Locked && !vertex1Locked)
				{
					entry.optimalPosition = positions[vertex0];
					entry.cost = combinedQuadric.Evaluate(positions[vertex0].x, positions[vertex0].y, positions[vertex0].z);
				}
				else if (!vertex0Locked && vertex1Locked)
				{
					entry.optimalPosition = positions[vertex1];
					entry.cost = combinedQuadric.Evaluate(positions[vertex1].x, positions[vertex1].y, positions[vertex1].z);
				}
				else
				{
					entry.optimalPosition = positions[vertex0];
					entry.cost = 1e30;
				}

				if (entry.cost < 0.0)
				{
					entry.cost = 0.0;
				}

				Uint64 key = makeEdgeKey(vertex0, vertex1);
				entry.version = ++edgeVersions[key];
				return entry;
			};

		std::priority_queue<EdgeEntry, DynamicArray<EdgeEntry>, std::greater<EdgeEntry>> edgeHeap;

		{
			std::set<Uint64> seenEdges;
			for (Size triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
			{
				Uint32 triangleVertices[3] =
				{
					indices[triangleIndex * 3],
					indices[triangleIndex * 3 + 1],
					indices[triangleIndex * 3 + 2]
				};
				for (Int edgeIndex = 0; edgeIndex < 3; edgeIndex++)
				{
					Uint64 key = makeEdgeKey(triangleVertices[edgeIndex], triangleVertices[(edgeIndex + 1) % 3]);
					if (seenEdges.insert(key).second)
					{
						edgeHeap.push(calculateEdgeCost(triangleVertices[edgeIndex], triangleVertices[(edgeIndex + 1) % 3]));
					}
				}
			}
		}

		Size aliveTriangleCount = triangleCount - degenerateTriangleCount;

		while (aliveTriangleCount > targetTriangleCount && !edgeHeap.empty())
		{
			EdgeEntry top = edgeHeap.top();
			edgeHeap.pop();

			Uint64 key = makeEdgeKey(top.vertex0, top.vertex1);
			if (edgeVersions[key] != top.version)
			{
				continue;
			}

			if (vertexDead[top.vertex0] || vertexDead[top.vertex1])
			{
				continue;
			}

			if (top.cost >= 1e29)
			{
				break;
			}

			Uint32 keepVertex = top.vertex0;
			Uint32 killVertex = top.vertex1;
			if (locked && locked[killVertex] && !locked[keepVertex])
			{
				std::swap(keepVertex, killVertex);
			}

			positions[keepVertex] = top.optimalPosition;
			quadrics[keepVertex] += quadrics[killVertex];
			vertexDead[killVertex] = true;

			std::set<Uint32> neighborVertices;

			for (Uint32 triangleIndex : vertexTriangles[killVertex])
			{
				if (triangleDead[triangleIndex])
				{
					continue;
				}

				Uint32& indexA = indices[triangleIndex * 3];
				Uint32& indexB = indices[triangleIndex * 3 + 1];
				Uint32& indexC = indices[triangleIndex * 3 + 2];

				if (indexA == killVertex)
				{
					indexA = keepVertex;
				}
				if (indexB == killVertex)
				{
					indexB = keepVertex;
				}
				if (indexC == killVertex)
				{
					indexC = keepVertex;
				}

				if (indexA == indexB || indexB == indexC || indexA == indexC)
				{
					triangleDead[triangleIndex] = true;
					aliveTriangleCount--;
				}
				else
				{
					vertexTriangles[keepVertex].push_back(triangleIndex);
					neighborVertices.insert(indexA);
					neighborVertices.insert(indexB);
					neighborVertices.insert(indexC);
				}
			}

			for (Uint32 neighborVertex : neighborVertices)
			{
				if (neighborVertex != keepVertex && !vertexDead[neighborVertex])
				{
					edgeHeap.push(calculateEdgeCost(keepVertex, neighborVertex));
				}
			}
		}

		Result result;

		DynamicArray<Uint32> remapTable(vertexCount, 0xFFFFFFFF);

		for (Size index = 0; index < vertexCount; index++)
		{
			if (!vertexDead[index])
			{
				remapTable[index] = static_cast<Uint32>(result.positions.size());
				result.positions.push_back(positions[index]);
				result.vertexMap.push_back(static_cast<Uint32>(index));
			}
		}

		for (Size triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
		{
			if (triangleDead[triangleIndex])
			{
				continue;
			}
			result.indices.push_back(remapTable[indices[triangleIndex * 3]]);
			result.indices.push_back(remapTable[indices[triangleIndex * 3 + 1]]);
			result.indices.push_back(remapTable[indices[triangleIndex * 3 + 2]]);
		}

		return result;
	}
}
