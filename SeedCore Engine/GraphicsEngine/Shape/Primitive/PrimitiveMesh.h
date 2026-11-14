#pragma once
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <GraphicsEngine/Model/Cluster/Meshlet.h>

namespace SeedCore
{
	class BindlessHeap;

	class PrimitiveMesh :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 kindCount_ = 4;

	public:
		PrimitiveMesh();

		~PrimitiveMesh();

		void Create(ID3D12Device* device, BindlessHeap& heap);

	public:
		NODISCARD Uint32 Slot(ShapeKind kind)const;

		NODISCARD const Meshlet::MeshletRange& Meshlets(Uint32 slot)const;

		NODISCARD Uint IndexBufferIndex(Uint32 slot)const;

		NODISCARD const D3D12_RAYTRACING_GEOMETRY_DESC& Geometry(Uint32 slot)const;

		NODISCARD Uint VertexBufferIndex()const;

		NODISCARD Uint MeshletBufferIndex()const;

		NODISCARD Uint MeshletBoundBufferIndex()const;

		NODISCARD Uint VertexIndicesBufferIndex()const;

		NODISCARD Uint PrimitiveIndicesBufferIndex()const;

	private:
		struct Point
		{
			Vector3 position_ = { 0.0f, 0.0f, 0.0f };

			Vector3 normal_ = { 0.0f, 0.0f, 1.0f };

			Vector2 texcoord_ = { 0.0f, 0.0f };
		};

	private:
		static void CreateBox(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles);

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
		static void CreateSphere(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles);

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
		static void CreateCylinder(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles);

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
		static void CreateCone(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles);

		Uint CreateBuffer(ID3D12Device* device, BindlessHeap& heap, const void* data, Uint64 size, Uint32 stride, Bool raw, Microsoft::WRL::ComPtr<ID3D12Resource>& resource);

	private:
		StaticArray<Meshlet::MeshletRange, kindCount_> meshlets_ = {};

		StaticArray<Uint, kindCount_> indexBufferIndices_ = {};

		StaticArray<D3D12_RAYTRACING_GEOMETRY_DESC, kindCount_> geometries_ = {};

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> meshletBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> meshletBoundBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexIndicesBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> primitiveIndicesBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> positionBuffer_;

		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;

		Uint vertexBufferIndex_ = SC_INVALID;

		Uint meshletBufferIndex_ = SC_INVALID;

		Uint meshletBoundBufferIndex_ = SC_INVALID;

		Uint vertexIndicesBufferIndex_ = SC_INVALID;

		Uint primitiveIndicesBufferIndex_ = SC_INVALID;
	};
}
