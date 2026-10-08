#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Log/Assert.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <GraphicsEngine/Model/Cluster/Meshlet.h>

namespace SeedCore
{
	struct PrimitiveVertex
	{
		Vector3 position_;
		Vector3 normal_;
		Vector2 texcoord_;
		Float cap_;
	};
	SC_STATIC_ASSERT(PrimitiveVertex, 36, "Shape/Primitive/Primitive.hlsli");

	class PrimitiveMesh :public NonCopyable
	{
	public:
		void Build();

	public:
		/// [EN] Most meshlets one shape may have: one amplification shader group of 32 threads tests one instance's meshlets.
		/// [JP] 形1つが持てるメッシュレットの最大数。32 スレッドの Amplification Shader の 1 グループが、1 インスタンスのメッシュレットを調べる。
		SC_CONST Uint32 maxMeshletsPerShape_ = 32;

	public:
		[[nodiscard]] std::span<const PrimitiveVertex> Vertices()const;

		[[nodiscard]] std::span<const MeshletDesc> Meshlets()const;

		[[nodiscard]] std::span<const MeshletBound> MeshletBounds()const;

		[[nodiscard]] std::span<const Meshlet::MeshletRange> MeshletRanges()const;

		[[nodiscard]] std::span<const Uint32> VertexIndices()const;

		[[nodiscard]] std::span<const Uint8> PrimitiveIndices()const;

	private:
		static void CreateBoxShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles);

	private:
		DynamicArray<PrimitiveVertex> vertices_;

		DynamicArray<MeshletDesc> meshlets_;

		DynamicArray<MeshletBound> meshletBounds_;

		DynamicArray<Meshlet::MeshletRange> meshletRanges_;

		DynamicArray<Uint32> vertexIndices_;

		DynamicArray<Uint8> primitiveIndices_;
	};
}