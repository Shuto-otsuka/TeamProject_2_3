#include <GraphicsEngine/Avatar/AvatarMesh.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	Bool AvatarMesh::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, std::span<const Uint32> triangles, std::span<const Uint32> skinIndices, std::span<const Float> skinWeights, std::span<const Vector2> texcoords, std::span<const Vector3> neutralPositions, Uint32 bodyVertexCount, std::span<const Uint32> regionTriangleRanges)
	{
		bodyVertexCount_ = bodyVertexCount;
		if (bodyVertexCount_ == 0 || neutralPositions.size() < bodyVertexCount_)
		{
			return false;
		}

		baseTexcoords_.assign(texcoords.begin(), texcoords.begin() + bodyVertexCount_);

		/// [EN] Build meshlets per region so no meshlet straddles a region boundary -
		///      the renderer can then draw one region (one texture) at a time.
		///      With one region or none, the whole mesh is one region.
		/// [JP] メッシュレットをリージョン単位で構築し、リージョン境界をまたがない
		///      ようにする - レンダラーがリージョン(=テクスチャ)ごとに描ける。
		///      リージョンが1つ以下なら、メッシュ全体を1つのリージョンにする。
		regionCount_ = Max(Min(static_cast<Uint32>(regionTriangleRanges.size() / 2), maxRegionCount_), 1u);

		Meshlet::TriangleRange triangleRanges[maxRegionCount_];
		for (Uint32 regionIndex = 0; regionIndex < regionCount_; regionIndex++)
		{
			triangleRanges[regionIndex].triangleOffset_ = regionCount_ > 1 ? regionTriangleRanges[regionIndex * 2 + 0] : 0;
			triangleRanges[regionIndex].triangleCount_ = regionCount_ > 1 ? regionTriangleRanges[regionIndex * 2 + 1] : static_cast<Uint32>(triangles.size() / 3);
		}

		/// [EN] Meshlet also pads the primitive indices to the 4-byte alignment ReadOnlyByteAddressBuffer needs.
		/// [JP] ReadOnlyByteAddressBuffer が必要とする4バイト境界へは Meshlet が揃える。
		Meshlet meshlet(triangles, std::span<const Meshlet::TriangleRange>(triangleRanges, regionCount_));
		meshlets_.assign(meshlet.Meshlets().begin(), meshlet.Meshlets().end());
		vertexIndices_.assign(meshlet.VertexIndices().begin(), meshlet.VertexIndices().end());
		primitiveIndices_.assign(meshlet.PrimitiveIndices().begin(), meshlet.PrimitiveIndices().end());
		for (Uint32 regionIndex = 0; regionIndex < regionCount_; regionIndex++)
		{
			regionMeshletRanges_[regionIndex].meshletOffset_ = meshlet.MeshletRanges()[regionIndex].meshletOffset_;
			regionMeshletRanges_[regionIndex].meshletCount_ = meshlet.MeshletRanges()[regionIndex].meshletCount_;
		}

		if (meshlets_.empty())
		{
			return false;
		}

		Uint32 alignedPrimitiveByteSize = static_cast<Uint32>(primitiveIndices_.size());

		skinVertices_.resize(bodyVertexCount_);
		for (Uint32 vertexIndex = 0; vertexIndex < bodyVertexCount_; vertexIndex++)
		{
			CompressedSkinVertex& skin = skinVertices_[vertexIndex];
			Uint32 joint0 = skinIndices[vertexIndex * 4 + 0];
			Uint32 joint1 = skinIndices[vertexIndex * 4 + 1];
			Uint32 joint2 = skinIndices[vertexIndex * 4 + 2];
			Uint32 joint3 = skinIndices[vertexIndex * 4 + 3];
			skin.jointsXY_ = (joint0 & 0xFFFF) | ((joint1 & 0xFFFF) << 16);
			skin.jointsZW_ = (joint2 & 0xFFFF) | ((joint3 & 0xFFFF) << 16);
			Float weight0 = Clamp(skinWeights[vertexIndex * 4 + 0], 0.0f, 1.0f);
			Float weight1 = Clamp(skinWeights[vertexIndex * 4 + 1], 0.0f, 1.0f);
			Float weight2 = Clamp(skinWeights[vertexIndex * 4 + 2], 0.0f, 1.0f);
			Float weight3 = Clamp(skinWeights[vertexIndex * 4 + 3], 0.0f, 1.0f);
			skin.weights_ =
				 static_cast<Uint32>(weight0 * 255.0f + 0.5f) |
				(static_cast<Uint32>(weight1 * 255.0f + 0.5f) << 8) |
				(static_cast<Uint32>(weight2 * 255.0f + 0.5f) << 16) |
				(static_cast<Uint32>(weight3 * 255.0f + 0.5f) << 24);
		}

		texcoordMin_ = Vector2(0.0f, 0.0f);
		texcoordExtent_ = Vector2(1.0f, 1.0f);
		if (!baseTexcoords_.empty())
		{
			Vector2 texcoordMax = baseTexcoords_[0];
			texcoordMin_ = baseTexcoords_[0];
			for (const Vector2& texcoord : baseTexcoords_)
			{
				texcoordMin_ = Vector2::Min(texcoordMin_, texcoord);
				texcoordMax = Vector2::Max(texcoordMax, texcoord);
			}
			texcoordExtent_ = Vector2::Max(texcoordMax - texcoordMin_, Vector2(1e-6f, 1e-6f));
		}

		Vector3 neutralMin = neutralPositions[0];
		Vector3 neutralMax = neutralPositions[0];
		for (Uint32 vertexIndex = 0; vertexIndex < bodyVertexCount_; vertexIndex++)
		{
			neutralMin = Vector3::Min(neutralMin, neutralPositions[vertexIndex]);
			neutralMax = Vector3::Max(neutralMax, neutralPositions[vertexIndex]);
		}
		Vector3 neutralExtent = neutralMax - neutralMin;
		positionMin_ = neutralMin - neutralExtent * 0.4f;
		positionExtent_ = Vector3::Max(neutralExtent * 1.8f, Vector3(1e-6f, 1e-6f, 1e-6f));

		scratchVertices_.resize(bodyVertexCount_);
		scratchBounds_.resize(meshlets_.size());

		vertexBuffer_ = MakePtr<ReadOnlyStructuredBuffer<CompressedVertex>>(device, bindlessHeap, bodyVertexCount_);
		skinVertexBuffer_ = MakePtr<ReadOnlyStructuredBuffer<CompressedSkinVertex>>(device, bindlessHeap, bodyVertexCount_);
		meshletBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MeshletDesc>>(device, bindlessHeap, static_cast<Uint>(meshlets_.size()));
		meshletBoundBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MeshletBound>>(device, bindlessHeap, static_cast<Uint>(meshlets_.size()));
		vertexIndicesBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Uint32>>(device, bindlessHeap, static_cast<Uint>(vertexIndices_.size()));
		primitiveIndicesBuffer_ = MakePtr<ReadOnlyByteAddressBuffer>(device, bindlessHeap, alignedPrimitiveByteSize);

		return true;
	}

	void AvatarMesh::Update(std::span<const Vector3> positions, std::span<const Vector3> normals)
	{
		if (!Created())
		{
			return;
		}

		if (positions.size() < bodyVertexCount_ || normals.size() < bodyVertexCount_)
		{
			return;
		}

		for (Uint32 vertexIndex = 0; vertexIndex < bodyVertexCount_; vertexIndex++)
		{
			Vertex vertex;
			vertex.position_ = positions[vertexIndex];
			vertex.normal_ = normals[vertexIndex];
			vertex.tangent_ = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
			vertex.texcoord_ = baseTexcoords_[vertexIndex];
			scratchVertices_[vertexIndex] = Crister::EncodeVertex(vertex, positionMin_, positionExtent_, texcoordMin_, texcoordExtent_);
		}

		MeshletBound bound;
		bound.center_ = positionMin_ + positionExtent_ * 0.5f;
		bound.radius_ = positionExtent_.Length() * 0.5f;
		bound.coneAxis_ = Vector3(0.0f, 0.0f, 1.0f);
		bound.coneCutoff_ = -1.0f;
		std::ranges::fill(scratchBounds_, bound);

		vertexBuffer_->Update(scratchVertices_.data(), static_cast<Uint>(scratchVertices_.size()));
		meshletBoundBuffer_->Update(scratchBounds_.data(), static_cast<Uint>(scratchBounds_.size()));
		skinVertexBuffer_->Update(skinVertices_.data(), static_cast<Uint>(skinVertices_.size()));
		meshletBuffer_->Update(meshlets_.data(), static_cast<Uint>(meshlets_.size()));
		vertexIndicesBuffer_->Update(vertexIndices_.data(), static_cast<Uint>(vertexIndices_.size()));
		primitiveIndicesBuffer_->Update(primitiveIndices_.data(), static_cast<Uint>(primitiveIndices_.size()));
	}

	Bool AvatarMesh::Created()const
	{
		return vertexBuffer_ != nullptr;
	}

	Uint AvatarMesh::VertexBufferIndex()const
	{
		return vertexBuffer_->Index();
	}

	Uint AvatarMesh::SkinVertexBufferIndex()const
	{
		return skinVertexBuffer_->Index();
	}

	Uint AvatarMesh::MeshletBufferIndex()const
	{
		return meshletBuffer_->Index();
	}

	Uint AvatarMesh::MeshletBoundBufferIndex()const
	{
		return meshletBoundBuffer_->Index();
	}

	Uint AvatarMesh::VertexIndicesBufferIndex()const
	{
		return vertexIndicesBuffer_->Index();
	}

	Uint AvatarMesh::PrimitiveIndicesBufferIndex()const
	{
		return primitiveIndicesBuffer_->Index();
	}

	Uint32 AvatarMesh::MeshletCount()const
	{
		return static_cast<Uint32>(meshlets_.size());
	}

	const AvatarMesh::RegionMeshletRange& AvatarMesh::MeshletRangeForRegion(Uint32 regionIndex)const
	{
		return regionMeshletRanges_[regionIndex < regionCount_ ? regionIndex : 0];
	}

	Uint32 AvatarMesh::RegionCount()const
	{
		return regionCount_;
	}

	Uint32 AvatarMesh::BodyVertexCount()const
	{
		return bodyVertexCount_;
	}

	Vector3 AvatarMesh::PositionMin()const
	{
		return positionMin_;
	}

	Vector3 AvatarMesh::PositionExtent()const
	{
		return positionExtent_;
	}

	Vector2 AvatarMesh::TexcoordMin()const
	{
		return texcoordMin_;
	}

	Vector2 AvatarMesh::TexcoordExtent()const
	{
		return texcoordExtent_;
	}
}
