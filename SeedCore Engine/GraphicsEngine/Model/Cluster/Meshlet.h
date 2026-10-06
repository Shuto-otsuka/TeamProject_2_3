#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	/**
	* [EN]
	* Mesh shader input for one meshlet: where its vertices start in the
	* vertex index table and how many there are, and where its triangles
	* start in the primitive index table (in bytes, three per triangle) and
	* how many there are. Each vertex index entry holds a global vertex
	* number; each primitive index byte holds a vertex number local to the
	* meshlet (0 to 63).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メッシュレット1つ分の、メッシュシェーダーへの入力。頂点番号の表の
	* どこから何個がこのメッシュレットの頂点か、三角形番号の表のどこから
	* （バイト単位、三角形1つにつき3バイト）何個が三角形かを持つ。頂点番号の
	* 表の各要素は全体での頂点番号、三角形番号の表の各バイトはメッシュレット内
	* での頂点番号（0〜63）。
	*/
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
		[[nodiscard]] std::span<const MeshletDesc> Meshlets()const;

		[[nodiscard]] std::span<const MeshletRange> MeshletRanges()const;

		[[nodiscard]] std::span<const Uint32> VertexIndices()const;

		[[nodiscard]] std::span<const Uint8> PrimitiveIndices()const;

	private:
		void Pack(std::span<const Uint32> triangles, std::span<const TriangleRange> triangleRanges);

	public:
		SC_CONST Uint32 maxVerticesPerMeshlet_ = 64;

		SC_CONST Uint32 maxTrianglesPerMeshlet_ = 124;

	private:
		DynamicArray<MeshletDesc> meshlets_;

		DynamicArray<MeshletRange> meshletRanges_;

		DynamicArray<Uint32> vertexIndices_;

		DynamicArray<Uint8> primitiveIndices_;
	};
}