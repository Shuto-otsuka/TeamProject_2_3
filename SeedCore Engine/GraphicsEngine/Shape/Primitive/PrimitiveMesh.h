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

		/**
		* [EN]
		* Builds the unit sphere: radius 1, centered on the origin, 16 rings
		* from the top pole (+Y) to the bottom pole and 32 segments around Y.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位の球を作る。半径 1 で原点が中心。上の極（+Y）から下の極まで
		* 16 段、Y 軸まわりに 32 分割。
		*/
		static void CreateSphereShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles);

		/**
		* [EN]
		* Builds the unit cylinder: radius 1 around Y, from y = -1 to y = 1,
		* with 32 segments around Y and a flat cap at each end.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位の円柱を作る。Y 軸まわりに半径 1、y = -1 から y = 1 まで。
		* Y 軸まわりに 32 分割し、両端に平らなフタを付ける。
		*/
		static void CreateCylinderShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles);

		/**
		* [EN]
		* Builds the unit cone: its base ring of radius 1 at y = -1, its apex
		* at y = 1, with 32 segments around Y and a flat cap on the base.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位の円錐を作る。半径 1 の底面の円が y = -1、頂点が y = 1。
		* Y 軸まわりに 32 分割し、底面に平らなフタを付ける。
		*/
		static void CreateConeShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles);

	private:
		DynamicArray<PrimitiveVertex> vertices_;

		DynamicArray<MeshletDesc> meshlets_;

		DynamicArray<MeshletBound> meshletBounds_;

		DynamicArray<Meshlet::MeshletRange> meshletRanges_;

		DynamicArray<Uint32> vertexIndices_;

		DynamicArray<Uint8> primitiveIndices_;
	};
}