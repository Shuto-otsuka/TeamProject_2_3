#pragma once
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>

namespace SeedCore
{
	class BindlessHeap;

	class SoftbodyMesh :public NonCopyable
	{
	public:
		SC_CONSTEX Uint32 bindMaxWeights_ = 4;

		SoftbodyMesh() = default;
		~SoftbodyMesh() = default;

		Bool Create(ID3D12Device* device, BindlessHeap* bindlessHeap, const Crister& crister);

		void Update(const DynamicArray<Vector3>& simulatedProxyPositions);

		NODISCARD Uint VertexBufferIndex()const;
		NODISCARD Uint MeshletBufferIndex()const;
		NODISCARD Uint MeshletBoundBufferIndex()const;
		NODISCARD Uint VertexIndicesBufferIndex()const;
		NODISCARD Uint PrimitiveIndicesBufferIndex()const;

		NODISCARD Uint32 MeshletCount()const;
		NODISCARD Vector3 PositionMin()const;
		NODISCARD Vector3 PositionExtent()const;
		NODISCARD Vector2 TexcoordMin()const;
		NODISCARD Vector2 TexcoordExtent()const;

	private:
		struct VertexBinding
		{
			Uint32 proxyIndex_[bindMaxWeights_] = {};
			Float weight_[bindMaxWeights_] = {};
		};

		static void BindRenderVerticesToProxy(const DynamicArray<Vertex>& renderVertices, const DynamicArray<Vertex>& proxyVertices, DynamicArray<VertexBinding>& outBindings);

		DynamicArray<Vertex> bindPoseVertices_;

		DynamicArray<Vertex> proxyBindPoseVertices_;

		DynamicArray<VertexBinding> renderVertexBindings_;

		DynamicArray<MeshletDesc> meshlets_;
		DynamicArray<Uint32> vertexIndices_;

		DynamicArray<Uint8> primitiveIndices_;

		Vector2 texcoordMin_ = { 0.0f, 0.0f };
		Vector2 texcoordExtent_ = { 1.0f, 1.0f };

		Vector3 positionMin_ = { 0.0f, 0.0f, 0.0f };
		Vector3 positionExtent_ = { 1.0f, 1.0f, 1.0f };

		DynamicArray<Vector3> scratchProxyDisplacements_;
		DynamicArray<Vector3> scratchDeformedPositions_;
		DynamicArray<CompressedVertex> scratchVertices_;
		DynamicArray<MeshletBound> scratchBounds_;

		ResourcePtr<ReadOnlyStructuredBuffer<CompressedVertex>> vertexBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<MeshletDesc>> meshletBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<MeshletBound>> meshletBoundBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<Uint32>> vertexIndicesBuffer_;
		ResourcePtr<ReadOnlyByteAddressBuffer> primitiveIndicesBuffer_;
	};
}
