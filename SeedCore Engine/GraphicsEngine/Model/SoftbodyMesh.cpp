#include <GraphicsEngine/Model/SoftbodyMesh.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	/**
	* [EN]
	* For each full-resolution render vertex, finds the bindMaxWeights_
	* nearest simulation-proxy vertices (by bind-pose distance) via brute
	* force (both lists are small enough that a KD-tree wouldn't pay for
	* itself at this one-time build cost) and weights them by inverse
	* squared distance, normalised to sum to 1.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フル解像度描画頂点ごとに、最も近いシミュレーション用プロキシ頂点
	* （バインドポーズ距離）を bindMaxWeights_ 個、総当たりで見つける
	* （どちらのリストも小さいため、この一度きりの構築コストで KD-tree は
	* 割に合わない）。距離の逆二乗で重み付けし、合計が1になるよう正規化する。
	*/
	void SoftbodyMesh::BindRenderVerticesToProxy(const DynamicArray<Vertex>& renderVertices, const DynamicArray<Vertex>& proxyVertices, DynamicArray<VertexBinding>& outBindings)
	{
		outBindings.resize(renderVertices.size());

		for (Size renderIndex = 0; renderIndex < renderVertices.size(); renderIndex++)
		{
			const Vector3& renderPosition = renderVertices[renderIndex].position_;

			Uint32 bestIndex[bindMaxWeights_] = {};
			Float bestDistanceSquared[bindMaxWeights_] = {};
			Uint32 bestCount = 0;

			for (Size proxyIndex = 0; proxyIndex < proxyVertices.size(); proxyIndex++)
			{
				Float distanceSquared = (proxyVertices[proxyIndex].position_ - renderPosition).LengthSquared();

				if (bestCount < bindMaxWeights_)
				{
					bestIndex[bestCount] = static_cast<Uint32>(proxyIndex);
					bestDistanceSquared[bestCount] = distanceSquared;
					bestCount++;
				}
				else
				{
					Uint32 worst = 0;
					for (Uint32 k = 1; k < bindMaxWeights_; k++)
					{
						if (bestDistanceSquared[k] > bestDistanceSquared[worst])
						{
							worst = k;
						}
					}

					if (distanceSquared < bestDistanceSquared[worst])
					{
						bestIndex[worst] = static_cast<Uint32>(proxyIndex);
						bestDistanceSquared[worst] = distanceSquared;
					}
				}
			}

			VertexBinding binding;
			Float weightSum = 0.0f;
			for (Uint32 k = 0; k < bindMaxWeights_; k++)
			{
				if (k < bestCount)
				{
					Float weight = 1.0f / (bestDistanceSquared[k] + 1e-6f);
					binding.proxyIndex_[k] = bestIndex[k];
					binding.weight_[k] = weight;
					weightSum += weight;
				}
				else
				{
					binding.proxyIndex_[k] = bestCount > 0 ? bestIndex[0] : 0;
					binding.weight_[k] = 0.0f;
				}
			}

			if (weightSum > 0.0f)
			{
				for (Uint32 k = 0; k < bindMaxWeights_; k++)
				{
					binding.weight_[k] /= weightSum;
				}
			}

			outBindings[renderIndex] = binding;
		}
	}

	Bool SoftbodyMesh::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, const Crister& crister)
	{
		DynamicArray<Uint32> indices;
		if (!crister.SoftbodyFinestVertices(bindPoseVertices_, indices))
		{
			return false;
		}

		DynamicArray<Uint32> proxyIndices;
		if (!crister.SoftbodyCoarsestVertices(proxyBindPoseVertices_, proxyIndices))
		{
			return false;
		}

		/// [EN] The whole render mesh is one group; Meshlet already pads the primitive indices to the 4-byte alignment ReadOnlyByteAddressBuffer needs.
		/// [JP] 描画メッシュ全体を1つのまとまりとして詰める。ReadOnlyByteAddressBuffer が必要とする4バイト境界へは Meshlet がすでに揃えている。
		Meshlet meshlet(indices);
		meshlets_.assign(meshlet.Meshlets().begin(), meshlet.Meshlets().end());
		vertexIndices_.assign(meshlet.VertexIndices().begin(), meshlet.VertexIndices().end());
		primitiveIndices_.assign(meshlet.PrimitiveIndices().begin(), meshlet.PrimitiveIndices().end());
		if (meshlets_.empty())
		{
			return false;
		}

		Uint32 alignedPrimitiveByteSize = static_cast<Uint32>(primitiveIndices_.size());

		texcoordMin_ = crister.TexcoordMin();
		texcoordExtent_ = crister.TexcoordExtent();

		BindRenderVerticesToProxy(bindPoseVertices_, proxyBindPoseVertices_, renderVertexBindings_);

		scratchProxyDisplacements_.resize(proxyBindPoseVertices_.size());
		scratchDeformedPositions_.resize(bindPoseVertices_.size());
		scratchVertices_.resize(bindPoseVertices_.size());
		scratchBounds_.resize(meshlets_.size());

		vertexBuffer_ = MakePtr<ReadOnlyStructuredBuffer<CompressedVertex>>(device, bindlessHeap, static_cast<Uint>(bindPoseVertices_.size()));
		meshletBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MeshletDesc>>(device, bindlessHeap, static_cast<Uint>(meshlets_.size()));
		meshletBoundBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MeshletBound>>(device, bindlessHeap, static_cast<Uint>(meshlets_.size()));
		vertexIndicesBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Uint32>>(device, bindlessHeap, static_cast<Uint>(vertexIndices_.size()));
		primitiveIndicesBuffer_ = MakePtr<ReadOnlyByteAddressBuffer>(device, bindlessHeap, alignedPrimitiveByteSize);

		return true;
	}

	void SoftbodyMesh::Update(const DynamicArray<Vector3>& simulatedProxyPositions)
	{
		if (proxyBindPoseVertices_.empty() || bindPoseVertices_.empty() || simulatedProxyPositions.size() != proxyBindPoseVertices_.size())
		{
			return;
		}

		for (Size proxyIndex = 0; proxyIndex < proxyBindPoseVertices_.size(); proxyIndex++)
		{
			scratchProxyDisplacements_[proxyIndex] = simulatedProxyPositions[proxyIndex] - proxyBindPoseVertices_[proxyIndex].position_;
		}

		for (Size renderIndex = 0; renderIndex < bindPoseVertices_.size(); renderIndex++)
		{
			const VertexBinding& binding = renderVertexBindings_[renderIndex];

			Vector3 blendedDisplacement(0.0f, 0.0f, 0.0f);
			for (Uint32 k = 0; k < bindMaxWeights_; k++)
			{
				blendedDisplacement += scratchProxyDisplacements_[binding.proxyIndex_[k]] * binding.weight_[k];
			}

			scratchDeformedPositions_[renderIndex] = bindPoseVertices_[renderIndex].position_ + blendedDisplacement;
		}

		Vector3 positionMax = scratchDeformedPositions_[0];
		positionMin_ = scratchDeformedPositions_[0];
		for (const Vector3& position : scratchDeformedPositions_)
		{
			positionMin_ = Vector3::Min(positionMin_, position);
			positionMax = Vector3::Max(positionMax, position);
		}
		positionExtent_ = Vector3::Max(positionMax - positionMin_, Vector3(1e-6f, 1e-6f, 1e-6f));

		for (Size vertexIndex = 0; vertexIndex < bindPoseVertices_.size(); vertexIndex++)
		{
			Vertex vertex = bindPoseVertices_[vertexIndex];
			vertex.position_ = scratchDeformedPositions_[vertexIndex];
			scratchVertices_[vertexIndex] = Crister::EncodeVertex(vertex, positionMin_, positionExtent_, texcoordMin_, texcoordExtent_);
		}

		/// [EN] One conservative bound (this frame's whole-mesh AABB)
		///      replicated to every meshlet — cheaper than per-meshlet
		///      bounds and still correct (each meshlet's geometry is a
		///      subset of the whole mesh, so the whole-mesh bound trivially
		///      contains it too). coneCutoff_ <= 0 disables ModelAS.hlsl's
		///      normal-cone culling (see its `bound.cone_cutoff_ > 0.0`
		///      guard) since per-meshlet cones aren't computed here.
		/// [JP] このフレームのメッシュ全体 AABB から得た1つの保守的な
		///      バウンドを、全メシュレットへ複製する — メシュレットごとの
		///      バウンドより安く、かつ正しい（各メシュレットのジオメトリは
		///      メッシュ全体の部分集合なので、メッシュ全体のバウンドは
		///      当然それも含む）。coneCutoff_ <= 0 は ModelAS.hlsl の
		///      法線コーンカリングを無効化する（`bound.cone_cutoff_ > 0.0`
		///      ガード参照）— メシュレットごとのコーンはここでは計算して
		///      いないため。
		MeshletBound bound;
		bound.center_ = (positionMin_ + positionMax) * 0.5f;
		bound.radius_ = (positionMax - positionMin_).Length() * 0.5f;
		bound.coneAxis_ = Vector3(0.0f, 0.0f, 1.0f);
		bound.coneCutoff_ = -1.0f;
		std::ranges::fill(scratchBounds_, bound);

		vertexBuffer_->Update(scratchVertices_.data(), static_cast<Uint>(scratchVertices_.size()));
		meshletBuffer_->Update(meshlets_.data(), static_cast<Uint>(meshlets_.size()));
		meshletBoundBuffer_->Update(scratchBounds_.data(), static_cast<Uint>(scratchBounds_.size()));
		vertexIndicesBuffer_->Update(vertexIndices_.data(), static_cast<Uint>(vertexIndices_.size()));
		primitiveIndicesBuffer_->Update(primitiveIndices_.data(), static_cast<Uint>(primitiveIndices_.size()));
	}

	Uint SoftbodyMesh::VertexBufferIndex()const
	{
		return vertexBuffer_->Index();
	}

	Uint SoftbodyMesh::MeshletBufferIndex()const
	{
		return meshletBuffer_->Index();
	}

	Uint SoftbodyMesh::MeshletBoundBufferIndex()const
	{
		return meshletBoundBuffer_->Index();
	}

	Uint SoftbodyMesh::VertexIndicesBufferIndex()const
	{
		return vertexIndicesBuffer_->Index();
	}

	Uint SoftbodyMesh::PrimitiveIndicesBufferIndex()const
	{
		return primitiveIndicesBuffer_->Index();
	}

	Uint32 SoftbodyMesh::MeshletCount()const
	{
		return static_cast<Uint32>(meshlets_.size());
	}

	Vector3 SoftbodyMesh::PositionMin()const
	{
		return positionMin_;
	}

	Vector3 SoftbodyMesh::PositionExtent()const
	{
		return positionExtent_;
	}

	Vector2 SoftbodyMesh::TexcoordMin()const
	{
		return texcoordMin_;
	}

	Vector2 SoftbodyMesh::TexcoordExtent()const
	{
		return texcoordExtent_;
	}
}
