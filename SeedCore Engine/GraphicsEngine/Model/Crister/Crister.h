#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>
#include <GraphicsEngine/Model/Crister/CristerScene.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <GraphicsEngine/Model/Crister/CristerResidency.h>

namespace SeedCore
{
	class SEEDCORE_API Crister
	{
	private:
		friend class ModelLoader;
		friend class ModelWriter;
		friend class CristerResidency;
		friend class CristerQuantizer;
		friend class CristerConverter;
		friend class CristerClusterizer;
		friend class CristerFlattener;
		friend class CristerExtractor;

		CristerGeometry geometry_;

		CristerScene scene_;

		CristerMaterial material_;

		CristerResidency residency_;

	public:
		Crister() = default;

		Crister(Crister&&)noexcept = default;

		Crister& operator=(Crister&&)noexcept = default;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("compressed_vertices", geometry_.compressedVertices_);
			archive.Field("compressed_skin_vertices", geometry_.compressedSkinVertices_);
			archive.Field("vertex_morph_source", geometry_.vertexMorphSource_);
			archive.Field("position_min", geometry_.positionMin_);
			archive.Field("position_extent", geometry_.positionExtent_);
			archive.Field("texcoord_min", geometry_.texcoordMin_);
			archive.Field("texcoord_extent", geometry_.texcoordExtent_);
			archive.Field("vertex_indices", geometry_.vertexIndices_);
			archive.Field("primitive_indices", geometry_.primitiveIndices_);
			archive.Field("meshlets", geometry_.meshlets_);
			archive.Field("meshlet_bounds", geometry_.meshletBounds_);
			archive.Field("clusters", geometry_.clusters_);
			archive.Field("stages", scene_.stages_);
			archive.Field("sub_meshes", geometry_.subMeshes_);
			archive.Field("surfaces", material_.surfaces_);
			archive.Field("nodes", scene_.nodes_);
			archive.Field("skins", scene_.skins_);
			archive.Field("bitmaps", material_.bitmaps_);
			archive.Field("lights", scene_.lights_);
			archive.Field("default_stage", scene_.defaultStage_);
		}

		NODISCARD CristerResidency& Residency();

		NODISCARD const CristerResidency& Residency()const;

		NODISCARD std::span<const Stage> Stages()const;

		NODISCARD std::span<const Node> Nodes()const;

		NODISCARD std::span<const PunctualLight> Lights()const;

		NODISCARD std::span<const SubMesh> SubMeshes()const;

		NODISCARD std::span<const Surface> Surfaces()const;

		NODISCARD std::span<const Bitmap> Bitmaps()const;

		NODISCARD std::span<const Skin> Skins()const;

		NODISCARD std::span<const Cluster> Clusters()const;

		NODISCARD Int DefaultStage()const;

		NODISCARD Int FindNodeIndex(const std::string& name)const;

		NODISCARD Vector3 PositionMin()const;

		NODISCARD Vector3 PositionExtent()const;

		NODISCARD Vector2 TexcoordMin()const;

		NODISCARD Vector2 TexcoordExtent()const;

	private:
		void CumulateTransforms();
	};
}