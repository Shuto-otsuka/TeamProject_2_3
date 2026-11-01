#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>
#include <GraphicsEngine/Model/Crister/CristerScene.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <GraphicsEngine/Model/Crister/CristerResidency.h>

namespace SeedCore
{
	/**
	* [EN]
	* Geometry/texture-streaming model asset: the quantised geometry
	* and its meshlet/cluster LOD hierarchy (geometry_), the stages,
	* nodes, skins and lights (scene_), the materials and embedded
	* images (material_), read from or written to a .crister cache, and
	* the GPU side built from them (residency_). ModelLoader fills it
	* through CristerClusterizer, CristerQuantizer, CristerCompressor and
	* CristerConverter; CristerExtractor reads its triangles back out on
	* the CPU, and CristerResidency (reached through Residency()) uploads
	* and streams it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジオメトリ/テクスチャをストリーミングするモデルのアセット。量子化
	* 済みのジオメトリと meshlet/cluster の LOD 階層(geometry_)、
	* ステージ・ノード・スキン・ライト(scene_)、マテリアルと内蔵画像
	* (material_)を持って .crister キャッシュと読み書きし、それらから
	* 作る GPU 側(residency_)も持つ。ModelLoader が CristerClusterizer、
	* CristerQuantizer、CristerCompressor、CristerConverter を通して中身を
	* 埋める。CristerExtractor が三角形を CPU 側へ取り出し、
	* CristerResidency(Residency() から触る)がアップロードと
	* ストリーミングを行う。
	*/
	class SEEDCORE_API Crister
	{
	private:
		friend class ModelLoader;
		friend class ModelExporter;
		friend class CristerResidency;
		friend class CristerQuantizer;
		friend class CristerConverter;
		friend class CristerClusterizer;
		friend class CristerFlattener;
		friend class CristerExtractor;

		/// [EN] Geometry: vertices, meshlets, clusters, SubMeshes and the quantisation AABBs.
		/// [JP] ジオメトリ。頂点、meshlet、クラスタ、SubMesh、量子化の AABB。
		CristerGeometry geometry_;

		/// [EN] Scene: stages, nodes, skins and lights.
		/// [JP] シーン。ステージ、ノード、スキン、ライト。
		CristerScene scene_;

		/// [EN] Materials and embedded images.
		/// [JP] マテリアルと内蔵画像。
		CristerMaterial material_;

		/// [EN] GPU resources and streaming state, created by CristerResidency::Upload().
		/// [JP] CristerResidency::Upload() で作る GPU リソースとストリーミング状態。
		CristerResidency residency_;

	public:
		/**
		* [EN]
		* Creates an empty Crister; ModelLoader fills it, or a .crister cache
		* is deserialised into it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空の Crister を作る。ModelLoader が中身を埋めるか、.crister
		* キャッシュをここへデシリアライズする。
		*/
		Crister() = default;

		/**
		* [EN]
		* Moves the model, GPU resources and streaming state included.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GPU リソースとストリーミング状態も含めて、モデルを移動する。
		*/
		Crister(Crister&&)noexcept = default;

		/**
		* [EN]
		* Move-assigns the model, GPU resources and streaming state included.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GPU リソースとストリーミング状態も含めて、モデルをムーブ代入する。
		*/
		Crister& operator=(Crister&&)noexcept = default;

		/**
		* [EN]
		* Reads or writes the baked model to or from a .crister cache.
		* vertices_ and the GPU-side state are not part of it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 焼き込み済みのモデルを .crister キャッシュと読み書きする。
		* vertices_ と GPU 側の状態は含まない。
		*/
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

		/**
		* [EN]
		* The GPU side of this Crister: its buffers, bindless indices and
		* geometry/texture streaming state. Functions that read the CPU
		* data as well take this Crister as an argument.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister の GPU 側。バッファ、bindless インデックス、
		* ジオメトリ/テクスチャのストリーミング状態を持つ。CPU 側のデータも
		* 読む関数は、この Crister を引数で受け取る。
		*/
		NODISCARD CristerResidency& Residency();

		/**
		* [EN]
		* Read-only access to the GPU side of this Crister.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister の GPU 側への読み取り専用のアクセス。
		*/
		NODISCARD const CristerResidency& Residency()const;

		/**
		* [EN]
		* Returns every Stage (root-node list + name) parsed from the source
		* glTF's scenes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソース glTF の scenes から解析された、全 Stage（ルートノード一覧 +
		* 名前）を返す。
		*/
		NODISCARD std::span<const Stage> Stages()const;

		/**
		* [EN]
		* Returns every Node in the flattened node hierarchy, including their
		* local S/R/T and cumulated globalTransform_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 平坦化されたノード階層の全 Node を返す。各ノードのローカル
		* S/R/T と、累積済みの globalTransform_ を含む。
		*/
		NODISCARD std::span<const Node> Nodes()const;

		/**
		* [EN]
		* Returns every KHR_lights_punctual point/spot light resolved from
		* the source glTF, in world space.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソース glTF から解決された、全 KHR_lights_punctual
		* ポイント/スポットライトをワールド空間で返す。
		*/
		NODISCARD std::span<const PunctualLight> Lights()const;

		/**
		* [EN]
		* Returns every SubMesh (material + cluster range + optional skin
		* index) this Crister is split into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister が分割されている全 SubMesh（マテリアル + クラスタ
		* 範囲 + 任意のスキンインデックス）を返す。
		*/
		NODISCARD std::span<const SubMesh> SubMeshes()const;

		/**
		* [EN]
		* Returns every Surface (PBR factors + KHR_materials_* extensions +
		* texture indices) parsed from the source glTF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソース glTF から解析された、全 Surface（PBR ファクタ +
		* KHR_materials_* 拡張 + テクスチャインデックス）を返す。
		*/
		NODISCARD std::span<const Surface> Surfaces()const;

		/**
		* [EN]
		* Returns every embedded Bitmap, indexed by the texture indices in
		* Surface. Each holds the BC7 mip chain baked by
		* CristerCompressor::BakeBitmap().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全ての内蔵 Bitmap を返す。Surface のテクスチャインデックスで
		* 引く。それぞれ CristerCompressor::BakeBitmap() が焼いた BC7 の
		* ミップチェーンを持つ。
		*/
		NODISCARD std::span<const Bitmap> Bitmaps()const;

		/**
		* [EN]
		* Returns every Skin (joint list + inverse-bind matrices) parsed
		* from the source glTF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソース glTF から解析された、全 Skin（ジョイント一覧 + 逆バインド
		* 行列）を返す。
		*/
		NODISCARD std::span<const Skin> Skins()const;

		/**
		* [EN]
		* Returns every Cluster (one LOD level's meshlet range within a
		* SubMesh) across all SubMeshes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全 SubMesh を通した、全 Cluster（SubMesh 内の1 LOD レベルぶんの
		* meshlet 範囲）を返す。
		*/
		NODISCARD std::span<const Cluster> Clusters()const;

		/**
		* [EN]
		* Returns the index into Stages() of the stage rendered by default
		* when no stage is explicitly selected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 明示的にステージが選択されていない時にデフォルトで描画される、
		* Stages() 内のインデックスを返す。
		*/
		NODISCARD Int DefaultStage()const;

		/**
		* [EN]
		* Linear search for the first Node whose name_ matches name, or -1
		* if none does. glTF node names are not guaranteed unique, so this
		* returns the first match in nodes_ order.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* name_ が name と一致する最初の Node を線形探索する、無ければ -1。
		* glTF のノード名は一意性が保証されないため、nodes_ の順で最初に
		* 見つかったものを返す。
		*/
		NODISCARD Int FindNodeIndex(const std::string& name)const;

		/**
		* [EN]
		* Minimum corner of the dequantisation AABB for CompressedVertex
		* positions (see the struct comment). Passed to the shaders through
		* ModelStructuredBuffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex 位置の逆量子化 AABB の最小コーナー（構造体コメント
		* 参照）。ModelStructuredBuffer 経由でシェーダに渡す。
		*/
		NODISCARD Vector3 PositionMin()const;

		/**
		* [EN]
		* Extent (max - min) of the dequantisation AABB for CompressedVertex
		* positions. Passed to the shaders through ModelStructuredBuffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex 位置の逆量子化 AABB の大きさ（max - min）。
		* ModelStructuredBuffer 経由でシェーダに渡す。
		*/
		NODISCARD Vector3 PositionExtent()const;

		/**
		* [EN]
		* Minimum corner of the dequantisation AABB for CompressedVertex
		* texcoords. Passed to the shaders through ModelStructuredBuffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex テクスチャ座標の逆量子化 AABB の最小コーナー。
		* ModelStructuredBuffer 経由でシェーダに渡す。
		*/
		NODISCARD Vector2 TexcoordMin()const;

		/**
		* [EN]
		* Extent (max - min) of the dequantisation AABB for CompressedVertex
		* texcoords. Passed to the shaders through ModelStructuredBuffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex テクスチャ座標の逆量子化 AABB の大きさ
		* （max - min）。ModelStructuredBuffer 経由でシェーダに渡す。
		*/
		NODISCARD Vector2 TexcoordExtent()const;

	private:
		/**
		* [EN]
		* Walks stages_[defaultStage_]'s node tree depth-first and
		* recomputes every Node::globalTransform_ from local S/R/T. Used by
		* ModelLoader while importing and by CristerConverter after a
		* conversion.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* stages_[defaultStage_]のノードツリーを深さ優先で走査し、ローカル
		* S/R/T から全 Node::globalTransform_ を再計算する。ModelLoader が
		* インポート中に、CristerConverter が変換の後に使う。
		*/
		void CumulateTransforms();
	};
}