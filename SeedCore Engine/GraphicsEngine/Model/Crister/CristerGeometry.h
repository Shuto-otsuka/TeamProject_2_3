#pragma once
#include <GraphicsEngine/Model/Cluster/Meshlet.h>

namespace SeedCore
{
	struct Vertex
	{
		Vector3 position_ = { 0,0,0 };

		Vector3 normal_ = { 0,0,1 };

		Vector4 tangent_ = { 1,0,0,1 };

		Vector2 texcoord_ = { 0,0 };

		Uvector4 joints_ = { 0,0,0,0 };

		Vector4 weights_ = { 1,0,0,0 };
	};

	struct CompressedVertex
	{
		Uint32 positionXY_ = 0;

		Uint32 positionZTexU_ = 0;

		Uint32 texVTangent_ = 0;

		Uint32 normal_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("position_xy", positionXY_);
			archive.Field("position_z_tex_u", positionZTexU_);
			archive.Field("tex_v_tangent", texVTangent_);
			archive.Field("normal", normal_);
		}
	};

	struct CompressedSkinVertex
	{
		Uint32 jointsXY_ = 0;

		Uint32 jointsZW_ = 0;

		Uint32 weights_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("joints_xy", jointsXY_);
			archive.Field("joints_zw", jointsZW_);
			archive.Field("weights", weights_);
		}
	};

	struct Cluster
	{
		Uint32 meshletOffset_ = 0;

		Uint32 meshletCount_ = 0;

		Uint32 lodLevel_ = 0;

		Float lodError_ = 0.0f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("meshlet_offset", meshletOffset_);
			archive.Field("meshlet_count", meshletCount_);
			archive.Field("lod_level", lodLevel_);
			archive.Field("lod_error", lodError_);
		}
	};

	struct Morph
	{
		std::string name_;

		DynamicArray<Vector3> positionDeltas_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("position_deltas", positionDeltas_);
		}
	};

	struct SubMesh
	{
		Uint32 clusterOffset_ = 0;

		Uint32 clusterCount_ = 0;

		Uint32 surfaceIndex_ = 0;

		Uint32 indexOffset_ = 0;

		Uint32 indexCount_ = 0;

		Int skinIndex_ = -1;

		Int meshIndex_ = -1;

		DynamicArray<Morph> morphs_;

		Uint32 vertexOffset_ = 0;

		Uint32 vertexCount_ = 0;

		Uint32 raytracingVertexOffset_ = 0;

		Uint32 raytracingVertexCount_ = 0;

		Uint32 raytracingTriangleOffset_ = 0;

		Uint32 raytracingTriangleCount_ = 0;

		Uint32 raytracingMorphDeltaOffset_ = SC_INVALID;

		Uint32 morphDeltaOffset_ = SC_INVALID;

		Float texcoordDensity_ = 0.0f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("cluster_offset", clusterOffset_);
			archive.Field("cluster_count", clusterCount_);
			archive.Field("surface_index", surfaceIndex_);
			archive.Field("index_offset", indexOffset_);
			archive.Field("index_count", indexCount_);
			archive.Field("skin_index", skinIndex_);
			archive.Field("mesh_index", meshIndex_);
			archive.Field("morphs", morphs_);
			archive.Field("vertex_offset", vertexOffset_);
			archive.Field("vertex_count", vertexCount_);
			archive.Field("texcoord_density", texcoordDensity_);
		}
	};

	enum class MeshCollisionDetail
	{
		Proxy,

		Exact,
	};

	struct CristerGeometry
	{
		DynamicArray<Vertex> vertices_;

		DynamicArray<Uint32> vertexMorphSource_;

		DynamicArray<CompressedVertex> compressedVertices_;

		DynamicArray<CompressedSkinVertex> compressedSkinVertices_;

		DynamicArray<Uint32> vertexIndices_;

		DynamicArray<Uint8> primitiveIndices_;

		DynamicArray<MeshletDesc> meshlets_;

		DynamicArray<MeshletBound> meshletBounds_;

		DynamicArray<Cluster> clusters_;

		DynamicArray<SubMesh> subMeshes_;

		Vector3 positionMin_ = { 0,0,0 };

		Vector3 positionExtent_ = { 1,1,1 };

		Vector2 texcoordMin_ = { 0,0 };

		Vector2 texcoordExtent_ = { 1,1 };
	};
}