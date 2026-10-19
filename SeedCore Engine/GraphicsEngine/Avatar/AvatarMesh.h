#pragma once
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>

namespace SeedCore
{
	class BindlessHeap;

	class SEEDCORE_API AvatarMesh :public NonCopyable
	{
	public:
		AvatarMesh() = default;
		~AvatarMesh() = default;

		struct RegionMeshletRange
		{
			Uint32 meshletOffset_ = 0;
			Uint32 meshletCount_ = 0;
		};

		Bool Create(ID3D12Device* device, BindlessHeap* bindlessHeap, std::span<const Uint32> triangles, std::span<const Uint32> skinIndices, std::span<const Float> skinWeights, std::span<const Vector2> texcoords, std::span<const Vector3> neutralPositions, Uint32 bodyVertexCount, std::span<const Uint32> regionTriangleRanges);

		NODISCARD const RegionMeshletRange& MeshletRangeForRegion(Uint32 regionIndex)const;

		NODISCARD Uint32 RegionCount()const;

		void Update(std::span<const Vector3> positions, std::span<const Vector3> normals);

		NODISCARD Bool Created()const;

		NODISCARD Uint VertexBufferIndex()const;
		NODISCARD Uint SkinVertexBufferIndex()const;
		NODISCARD Uint MeshletBufferIndex()const;
		NODISCARD Uint MeshletBoundBufferIndex()const;
		NODISCARD Uint VertexIndicesBufferIndex()const;
		NODISCARD Uint PrimitiveIndicesBufferIndex()const;

		NODISCARD Uint32 MeshletCount()const;
		NODISCARD Uint32 BodyVertexCount()const;
		NODISCARD Vector3 PositionMin()const;
		NODISCARD Vector3 PositionExtent()const;
		NODISCARD Vector2 TexcoordMin()const;
		NODISCARD Vector2 TexcoordExtent()const;

	private:
		SC_CONSTEX Uint32 maxRegionCount_ = 4;

		Uint32 bodyVertexCount_ = 0;
		Uint32 regionCount_ = 0;
		RegionMeshletRange regionMeshletRanges_[maxRegionCount_];

		DynamicArray<Vector2> baseTexcoords_;

		DynamicArray<MeshletDesc> meshlets_;
		DynamicArray<Uint32> vertexIndices_;
		DynamicArray<Uint8> primitiveIndices_;
		DynamicArray<CompressedSkinVertex> skinVertices_;

		Vector2 texcoordMin_ = { 0.0f, 0.0f };
		Vector2 texcoordExtent_ = { 1.0f, 1.0f };
		Vector3 positionMin_ = { 0.0f, 0.0f, 0.0f };
		Vector3 positionExtent_ = { 1.0f, 1.0f, 1.0f };

		DynamicArray<CompressedVertex> scratchVertices_;
		DynamicArray<MeshletBound> scratchBounds_;

		ResourcePtr<ReadOnlyStructuredBuffer<CompressedVertex>> vertexBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<CompressedSkinVertex>> skinVertexBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<MeshletDesc>> meshletBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<MeshletBound>> meshletBoundBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<Uint32>> vertexIndicesBuffer_;
		ResourcePtr<ReadOnlyByteAddressBuffer> primitiveIndicesBuffer_;
	};
}
