#pragma once
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	struct MeshletDesc
	{
		Uint32 vertexOffset_ = 0;
		Uint32 triangleOffset_ = 0;
		Uint32 vertexCount_ = 0;
		Uint32 triangleCount_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("vertex_offset", vertexOffset_);
			archive.Field("triangle_offset", triangleOffset_);
			archive.Field("vertex_count", vertexCount_);
			archive.Field("triangle_count", triangleCount_);
		}
	};
	SC_STATIC_ASSERT(MeshletDesc, 16, "Model/Model.hlsli");

	struct MeshletBound
	{
		Vector3 center_ = { 0,0,0 };
		Float radius_ = 0.0f;
		Vector3 coneAxis_ = { 0,0,1 };
		Float coneCutoff_ = 1.0f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("center", center_);
			archive.Field("radius", radius_);
			archive.Field("cone_axis", coneAxis_);
			archive.Field("cone_cutoff", coneCutoff_);
		}
	};
	SC_STATIC_ASSERT(MeshletBound, 32, "Model/Model.hlsli");

	class Meshlet
	{
	public:
		struct TriangleRange
		{
			Uint32 triangleOffset_ = 0;
			Uint32 triangleCount_ = 0;
		};

		struct MeshletRange
		{
			Uint32 meshletOffset_ = 0;
			Uint32 meshletCount_ = 0;
		};

	public:
		Meshlet(std::span<const Uint32> triangles);

		Meshlet(std::span<const Uint32> triangles, std::span<const TriangleRange> triangleRanges);

	public:
		NODISCARD std::span<const MeshletDesc> Meshlets()const;

		NODISCARD std::span<const MeshletRange> MeshletRanges()const;

		NODISCARD std::span<const Uint32> VertexIndices()const;

		NODISCARD std::span<const Uint8> PrimitiveIndices()const;

	private:
		void Pack(std::span<const Uint32> triangles, std::span<const TriangleRange> triangleRanges);

	public:
		SC_CONSTEX Uint32 maxVerticesPerMeshlet_ = 64;

		SC_CONSTEX Uint32 maxTrianglesPerMeshlet_ = 124;

	private:
		DynamicArray<MeshletDesc> meshlets_;

		DynamicArray<MeshletRange> meshletRanges_;

		DynamicArray<Uint32> vertexIndices_;

		DynamicArray<Uint8> primitiveIndices_;
	};
}