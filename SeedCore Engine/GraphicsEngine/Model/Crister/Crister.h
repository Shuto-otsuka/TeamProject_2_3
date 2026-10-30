#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>
#include <GraphicsEngine/Model/Crister/CristerScene.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <GraphicsEngine/Model/Crister/CristerGpu.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12CommandQueue;

	/**
	* [EN]
	* Geometry/texture-streaming model asset: the quantised geometry
	* and its meshlet/cluster LOD hierarchy (geometry_), the stages,
	* nodes, skins and lights (scene_), and the materials and embedded
	* images (material_), read from or written to a .crister cache.
	* ModelLoader fills it through CristerClusterizer, CristerQuantizer,
	* CristerCompressor and CristerConverter, and Upload() creates its
	* GPU side (gpu_, see CristerGpu). The GPU functions here forward to
	* gpu_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジオメトリ/テクスチャをストリーミングするモデルのアセット。量子化
	* 済みのジオメトリと meshlet/cluster の LOD 階層(geometry_)、
	* ステージ・ノード・スキン・ライト(scene_)、マテリアルと内蔵画像
	* (material_)を持ち、.crister キャッシュと読み書きする。ModelLoader
	* が CristerClusterizer、CristerQuantizer、CristerCompressor、
	* CristerConverter を通して中身を埋め、Upload() が GPU 側(gpu_、
	* CristerGpu 参照)を作る。ここの GPU 関連の関数は gpu_ へ転送する。
	*/
	class SEEDCORE_API Crister
	{
	private:
		friend class ModelLoader;
		friend class ModelExporter;
		friend class CristerGpu;
		friend class CristerQuantizer;
		friend class CristerConverter;
		friend class CristerClusterizer;
		friend class CristerFlattener;

		/// [EN] Geometry: vertices, meshlets, clusters, SubMeshes and the quantisation AABBs.
		/// [JP] ジオメトリ。頂点、meshlet、クラスタ、SubMesh、量子化の AABB。
		CristerGeometry geometry_;

		/// [EN] Scene: stages, nodes, skins and lights.
		/// [JP] シーン。ステージ、ノード、スキン、ライト。
		CristerScene scene_;

		/// [EN] Materials and embedded images.
		/// [JP] マテリアルと内蔵画像。
		CristerMaterial material_;

		/// [EN] GPU resources and streaming state, created by Upload().
		/// [JP] Upload() で作る GPU リソースとストリーミング状態。
		CristerGpu gpu_;

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
		* Flattens this Crister's triangles into a CPU-side position/index
		* pair, for MeshCollisionLoader to bake into a ".collision" file.
		* Reads only the CPU-resident arrays (compressedVertices_/
		* meshlets_/vertexIndices_/primitiveIndices_/clusters_), so it is
		* unaffected by geometry streaming residency and needs no GPU
		* readback. Positions are dequantised with the same math as the
		* shader decode. Returns false when there is nothing to extract.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister の三角形を CPU 側の位置/インデックス対へ展開する。
		* MeshCollisionLoader がこれを ".collision" ファイルへ焼き込む。
		* CPU 常駐配列 (compressedVertices_/meshlets_/vertexIndices_/
		* primitiveIndices_/clusters_) しか読まないため、ジオメトリ
		* ストリーミングの常駐状態に影響されず、GPU リードバックも不要。
		* 位置はシェーダデコードと同じ計算で逆量子化する。抽出対象が
		* 無ければ false を返す。
		*/
		NODISCARD Bool BakeCollision(MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const;

		/**
		* [EN]
		* Inverse of BakeMesh: decodes compressedVertices_ / compressedSkinVertices_
		* back into vertices_ and rebuilds vertexIndices_ / subMeshes_ as a flat
		* per-SubMesh triangle list from the LOD 0 meshlets. For a ".crister"-only
		* asset (no source glTF/FBX to re-parse) this is how ModelExporter gets an
		* uncompressed, non-meshlet mesh to export.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* BakeMesh の逆: compressedVertices_ / compressedSkinVertices_ を
		* vertices_ へデコードし直し、LOD 0 メシュレットから vertexIndices_ /
		* subMeshes_ を SubMesh ごとのフラットな三角形リストへ再構築する。
		* ".crister" 単体アセット（再解析できるソース glTF/FBX が無い）を
		* ModelExporter が書き出すための、非圧縮・非メシュレットのメッシュを
		* 得る手段。
		*/
		void Reconstruct();

		/**
		* [EN]
		* Reads this Crister's LOD 0 geometry into a CPU-side full-vertex
		* (position/normal/tangent/texcoord)/index pair addressed by the
		* SAME global vertex index space compressedVertices_/vertexIndices_
		* already use — decoded from compressedVertices_ (there is no
		* surviving full-precision source once BakeMesh() has run). Unlike
		* BakeCollision (which compacts/dedups into its own local index
		* space and is baked once into the cached ".collision" asset), this
		* is not part of the bake/cache pipeline — nothing is written to
		* disk, and Softbody calls it directly off the already-loaded
		* Crister once, to seed both the physics simulation (positions) and
		* the render-side SoftbodyMesh (normal_/tangent_/texcoord_, which
		* stay at their bind pose — the simulation only ever overwrites
		* position_ each frame; see SoftbodyMesh). outVertices is a 1:1,
		* unfiltered copy of compressedVertices_ and outIndices reference it
		* directly, so a physical simulation can run on the exact same
		* vertex ordering the mesh shader reads and write simulated vertex i
		* straight back into the render vertex buffer at index i with no
		* remap step. Reads only the CPU-resident arrays, same as
		* BakeCollision. Returns false when there is nothing to extract.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister の LOD 0 ジオメトリを、compressedVertices_/
		* vertexIndices_ が既に使っているのと同じグローバル頂点インデックス
		* 空間で、CPU 側のフル頂点（位置/法線/接線/UV）/インデックス対へ
		* 読み出す — compressedVertices_ からのデコードになる（BakeMesh()
		* 実行後はフル精度のソースが残っていないため）。BakeCollision
		* （独自のローカルインデックス空間へ圧縮・重複排除し、".collision"
		* アセットとして一度だけ焼き込む）と異なり、これはベイク/キャッシュ
		* パイプラインの一部ではない — ディスクには何も書き込まず、
		* Softbody が一度だけロード済みの Crister に対して直接呼び出し、
		* 物理シミュレーション（位置）とレンダー側の SoftbodyMesh
		* （normal_/tangent_/texcoord_ — バインドポーズのまま使い回す。
		* シミュレーションは毎フレーム position_ だけを上書きする。
		* SoftbodyMesh 参照）の両方の種にする。outVertices は
		* compressedVertices_ の 1:1 な無加工コピーで、outIndices はそれを
		* 直接参照する。これにより物理シミュレーションがメッシュシェーダと
		* 全く同じ頂点順序で動作でき、シミュレート後の頂点 i をそのまま
		* レンダー用頂点バッファのインデックス i へ書き戻せる（リマップ
		* 不要）。BakeCollision と同じく CPU 常駐配列のみを読む。抽出対象が
		* 無ければ false を返す。
		*/
		NODISCARD Bool SoftbodyFinestVertices(DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const;

		/**
		* [EN]
		* Reads each SubMesh's coarsest cluster (the same range as
		* BakeCollision's Proxy detail) into a CPU-side
		* full-vertex/index pair, compacted/deduped into its own local
		* index space (unlike SoftbodyFinestVertices, which keeps the full LOD 0
		* global index space uncompacted). For a Softbody to stay real-time
		* on a render-resolution mesh (thousands of vertices, one PBD edge/
		* shear/bend constraint per edge — far past what Jolt's soft body
		* solver is meant for), simulation runs on this much smaller proxy
		* mesh instead; SoftbodyMesh binds each render vertex (from
		* SoftbodyFinestVertices) to its nearest proxy vertices at build time and
		* blends their simulated displacement onto the full-resolution mesh
		* every frame, so the mesh shader still renders full detail. Not
		* part of the bake/cache pipeline — called live off the
		* already-loaded Crister. Returns false when there is nothing to
		* extract.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各 SubMesh の最粗クラスタ（BakeCollision の Proxy 詳細度と同じ
		* 範囲）を、CPU 側のフル頂点/インデックス対
		* へ、自身のローカルインデックス空間に圧縮・重複排除して読み出す
		* （LOD 0 のグローバルインデックス空間を無加工のまま保つ
		* SoftbodyFinestVertices とは異なる）。Softbody がレンダー解像度の
		* メッシュ（数千頂点、辺ごとに PBD の辺/シア/曲げ拘束1つ —
		* Jolt のソフトボディソルバーが想定する規模をはるかに超える）で
		* リアルタイムを保つため、シミュレーションはこのずっと小さい
		* プロキシメッシュ上で行う。SoftbodyMesh がビルド時に各レンダー
		* 頂点（SoftbodyFinestVertices 由来）を最も近いプロキシ頂点群へ束縛し、
		* 毎フレームそのシミュレート済み変位をブレンドしてフル解像度
		* メッシュへ適用するので、メッシュシェーダは引き続きフル
		* ディテールで描画する。ベイク/キャッシュパイプラインの一部では
		* なく、ロード済みの Crister に対してその都度呼び出す。抽出対象が
		* 無ければ false を返す。
		*/
		NODISCARD Bool SoftbodyCoarsestVertices(DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const;

		/**
		* [EN]
		* Inverse of EncodeVertex: dequantises a CompressedVertex back into a
		* full-precision Vertex, against this Crister's own quantisation AABBs
		* (positionMin_/positionExtent_/texcoordMin_/texcoordExtent_) — unlike
		* EncodeVertex, not static, since every call site decodes this
		* Crister's own compressedVertices_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* EncodeVertex の逆。CompressedVertex を、この Crister 自身の量子化
		* AABB（positionMin_/positionExtent_/texcoordMin_/texcoordExtent_）に
		* 対してフル精度の Vertex へ逆量子化する。EncodeVertex と異なり
		* static ではない — どの呼び出し元もこの Crister 自身の
		* compressedVertices_ をデコードするため。
		*/
		NODISCARD Vertex DecodeVertex(const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Inverse of EncodeOctahedralNormal (Shader/Normal.hlsli::OctNormalDecode's
		* CPU-side counterpart). Not static, matching DecodeVertex.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* EncodeOctahedralNormal の逆（Shader/Normal.hlsli::OctNormalDecode の
		* CPU 側対応）。DecodeVertex と同じく static ではない。
		*/
		NODISCARD Vector3 DecodeOctahedralNormal(Uint32 packed)const;

		/**
		* [EN]
		* Creates every GPU resource this Crister needs to draw: derives each
		* Cluster's streaming page ranges from its meshlet slice and pins the
		* pages that must never evict (coarsest cluster per SubMesh, skinned
		* LOD 0, the vertex pool they reference), uploads skin attributes for
		* the pool range, flattens each SubMesh's LOD 0 cluster, once per node
		* placement, into RT proxy geometry (compressed vertices + decoded
		* float3 positions + flat 32-bit triangle indices, deduplicated),
		* resolves SubMeshPlacement from the nodes, sets up per-texture
		* streaming state from BakeBitmap()'s BC7 mip chains (clamped to the
		* leading run of mip levels legal as a standalone single-mip BC7
		* resource), then registers with the shared streaming bookkeeping and
		* brings the pinned geometry pages/texture mips resident so the model
		* is immediately drawable at its fallback LOD/resolution. Finer
		* pages/mips stream in later on demand (MakeClusterResident/
		* MakeTextureMipResident, driven by ModelRenderer::Gather).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister の描画に必要な全 GPU リソースを作成する: 各 Cluster の
		* ストリーミングページ範囲を meshlet スライスから導出し、追い出しては
		* いけないページ（SubMesh ごとの最粗クラスタ、スキンド LOD 0、それらが
		* 参照する頂点プール）をピン留めする。プール範囲分のスキニング属性を
		* アップロードし、各 SubMesh の LOD 0 クラスタをノード配置ごとに RT
		* プロキシジオメトリ（圧縮頂点 + デコード済み float3 位置 + 重複排除
		* 済みフラット 32bit 三角形インデックス）へ展開し、ノードから
		* SubMeshPlacement を解決する。BakeBitmap() が焼いた BC7 ミップ
		* チェーンから（「単一ミップの独立した BC7 リソース」として合法な
		* 先頭のミップ範囲へ切り詰めた上で）テクスチャごとのストリーミング
		* 状態を準備し、最後に共有ストリーミング管理へ登録してピン留め
		* ジオメトリページ/テクスチャミップを常駐させ、フォールバック
		* LOD/解像度で即座に描画可能にする。より細かいページ/ミップは後で
		* オンデマンドにストリームインする（MakeClusterResident/
		* MakeTextureMipResident、ModelRenderer::Gather から駆動）。
		*/
		void Upload(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap);

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
		* Minimum corner of the model-space bounds of the geometry after
		* every SubMesh is placed by its nodes (SubMeshPlacement), taken
		* from the RT proxy positions in Upload(). Equals PositionMin()
		* when there is no proxy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各 SubMesh をノードで配置した(SubMeshPlacement)後のジオメトリの、
		* モデル空間での境界の最小コーナー。Upload() で RT プロキシの位置から
		* 求める。プロキシが無ければ PositionMin() と同じ。
		*/
		NODISCARD Vector3 PlacedPositionMin()const;

		/**
		* [EN]
		* Extent (max - min) of the same placed model-space bounds. Equals
		* PositionExtent() when there is no proxy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 同じ配置後のモデル空間での境界の大きさ（max - min）。プロキシが
		* 無ければ PositionExtent() と同じ。
		*/
		NODISCARD Vector3 PlacedPositionExtent()const;

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

		/**
		* [EN]
		* GPU address of the RT proxy's dedicated float3 position buffer
		* (stride = sizeof(Vector3)) for BLAS construction, decoded on the
		* CPU from the quantised CompressedVertex data with the same math
		* the mesh shader uses — so BLAS positions match the rasterized ones.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* BLAS 構築用の、RT プロキシ専用 float3 位置バッファ
		* （stride = sizeof(Vector3)）の GPU アドレス。量子化済み
		* CompressedVertex からメッシュシェーダと同じ計算で CPU デコード
		* するため、BLAS の位置はラスタライズ結果と一致する。
		*/
		NODISCARD D3D12_GPU_VIRTUAL_ADDRESS PositionBufferAddress()const;

		/**
		* [EN]
		* Bindless SRV of the same float3 position buffer, read by the morph
		* and skin blend passes as their base positions.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 同じ float3 位置バッファの bindless SRV。モーフとスキンのブレンド
		* パスがベース位置として読む。
		*/
		NODISCARD Uint PositionBufferIndex()const;

		/**
		* [EN]
		* Vertex count of the RT proxy's compact position/vertex buffers
		* (positionResource_/vertexResource_) PositionBufferAddress() points
		* to, i.e. the size a morph-blend scratch position buffer must be
		* allocated to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* PositionBufferAddress() が指す RT プロキシのコンパクトな位置/
		* 頂点バッファ(positionResource_/vertexResource_)の頂点数。
		* モーフブレンド用の一時位置バッファを確保すべきサイズでもある。
		*/
		NODISCARD Uint32 VertexCount()const;

		/**
		* [EN]
		* GPU address of the flat (non-meshlet) 32-bit triangle index buffer
		* for BLAS construction, unpacked once at Upload() time from
		* primitiveIndices_/vertexIndices_ in the same order the mesh shader
		* draws them — so BLAS geometry matches the rasterized geometry
		* exactly (no re-derived winding).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* BLAS 構築用の、フラット（非meshlet）32bit 三角形インデックス
		* バッファの GPU アドレス。primitiveIndices_/vertexIndices_ から
		* Upload() 時に1度だけ、メッシュシェーダが描くのと同じ順序で展開する
		* — BLAS のジオメトリはラスタライズされるジオメトリと完全に一致する
		* （巻き順を再導出しない）。
		*/
		NODISCARD D3D12_GPU_VIRTUAL_ADDRESS IndexBufferAddress()const;

		/**
		* [EN]
		* Triangle count of the flat index buffer IndexBufferAddress() points to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* IndexBufferAddress() が指すフラットインデックスバッファの三角形数。
		*/
		NODISCARD Uint32 IndexCount()const;


		/**
		* [EN]
		* Maps a glTF image index (as stored in Surface) to the bindless
		* heap index of the uploaded GPU texture. Returns 0xFFFFFFFF when
		* not present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* glTF の image インデックス（Surface に格納されている値）を、
		* アップロード済み GPU テクスチャの bindless ヒープインデックスに
		* 変換する。無ければ 0xFFFFFFFF。
		*/
		NODISCARD Uint TextureBindlessIndex(Uint32 textureIndex)const;

		/**
		* [EN]
		* Whether any texture's resident bindless index has changed (via
		* MakeTextureMipResident/EvictTextureMip) since the last
		* ClearMaterialsDirty() - RaytracingRenderer checks this to know
		* whether its cached reflection/GI material table (which baked
		* TextureBindlessIndex() at build time) needs rebuilding.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の ClearMaterialsDirty() 以降に、いずれかのテクスチャの常駐
		* バインドレスインデックスが(MakeTextureMipResident/EvictTextureMip
		* 経由で)変わったか。RaytracingRenderer がこれを見て、キャッシュ済みの
		* 反射/GI マテリアルテーブル(構築時に TextureBindlessIndex() を
		* 焼き込んでいる)を再構築すべきか判断する。
		*/
		NODISCARD Bool MaterialsDirty()const;

		/**
		* [EN]
		* Clears the flag MaterialsDirty() reports, after the caller has
		* rebuilt whatever depended on the stale bindless indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* MaterialsDirty() が報告するフラグをクリアする。呼び出し側が、
		* 古いバインドレスインデックスに依存していたものを再構築した後に呼ぶ。
		*/
		void ClearMaterialsDirty()const;

		/**
		* [EN]
		* Bindless SRV of the RT proxy CompressedVertex buffer, built from
		* each SubMesh's LOD 0 cluster so reflections show the mesh at full
		* detail. The proxy stays resident independently of raster
		* streaming.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* RT プロキシ CompressedVertex バッファの bindless SRV。反射に
		* メッシュをフル詳細で映すため、各 SubMesh の LOD 0 クラスタから
		* 構築する。プロキシはラスタ側のストリーミングとは関係なく常駐する。
		*/
		NODISCARD Uint VertexBufferIndex()const;

		/**
		* [EN]
		* Bindless SRV over the flat 32-bit triangle index buffer, for
		* raytracing closesthit shaders to re-fetch the hit triangle's
		* vertices (PrimitiveIndex() * 3 + 0/1/2 -> vertex index).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フラット 32bit 三角形インデックスバッファの bindless SRV。
		* レイトレの closesthit がヒット三角形の頂点を引き直すのに使う
		* (PrimitiveIndex() * 3 + 0/1/2 → 頂点インデックス)。
		*/
		NODISCARD Uint IndexBufferIndex()const;

		/**
		* [EN]
		* Bindless SRV of the CompressedSkinVertex buffer, or 0xFFFFFFFF
		* when this Crister has no skins.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedSkinVertex バッファの bindless SRV。スキンが無い
		* Crister では 0xFFFFFFFF。
		*/
		NODISCARD Uint SkinVertexBufferIndex()const;

		/**
		* [EN]
		* Bindless SRV of morphDeltaResource_ (raster-side flat morph
		* target delta pool, target-major per SubMesh — see
		* SubMesh::morphDeltaOffset_), or 0xFFFFFFFF when no SubMesh has
		* morphs_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* morphDeltaResource_(ラスタ側のフラットなモーフターゲットデルタ
		* プール、SubMesh ごとのターゲット主順 — SubMesh::morphDeltaOffset_
		* 参照)の bindless SRV。どの SubMesh も morphs_ を持たなければ
		* 0xFFFFFFFF。
		*/
		NODISCARD Uint MorphDeltaBufferIndex()const;

		/**
		* [EN]
		* Bindless SRV of vertexMorphSourceResource_ (per-vertex remap to
		* the original vertex morphDeltaResource_ is indexed by — see
		* vertexMorphSource_'s comment), or 0xFFFFFFFF when no SubMesh has
		* morphs_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* vertexMorphSourceResource_(morphDeltaResource_ がインデックスする
		* オリジナル頂点への、頂点ごとの逆引き — vertexMorphSource_ の
		* コメント参照)の bindless SRV。どの SubMesh も morphs_ を持たなければ
		* 0xFFFFFFFF。
		*/
		NODISCARD Uint VertexMorphSourceBufferIndex()const;

		/**
		* [EN]
		* Whether this Crister has RT-side skinning data (skinVertexResource_/
		* raytracingSkinVertexResource_ populated), i.e. skins_ is non-empty and the
		* RT proxy build found at least one skinned SubMesh.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Crister が RT 側のスキニングデータを持つか
		* (skinVertexResource_/raytracingSkinVertexResource_ が構築済みか)。
		* skins_ が空でなく、RT プロキシ構築時にスキンド SubMesh を
		* 1つ以上見つけた場合に true。
		*/
		NODISCARD Bool ProxySkinned()const;

		/**
		* [EN]
		* Bindless SRV of the RT proxy's skin vertex pool
		* (raytracingSkinVertexResource_), or SC_INVALID when ProxySkinned is
		* false.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* RT プロキシのスキン頂点プール (raytracingSkinVertexResource_) の
		* bindless SRV。ProxySkinned が false なら SC_INVALID。
		*/
		NODISCARD Uint ProxySkinVertexBufferIndex()const;

		/**
		* [EN]
		* Bindless SRV of the RT proxy's morph delta pool
		* (raytracingMorphDeltaResource_), or SC_INVALID when no SubMesh has
		* morphs_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* RT プロキシのモーフデルタプール (raytracingMorphDeltaResource_) の
		* bindless SRV。どの SubMesh も morphs_ を持たなければ SC_INVALID。
		*/
		NODISCARD Uint ProxyMorphDeltaBufferIndex()const;

		/**
		* [EN]
		* Returns the model-space placements of one SubMesh: the
		* globalTransform_ of every Node whose mesh_ references the
		* SubMesh's meshIndex_, so a mesh instanced by several nodes is
		* drawn once per node. Skinned SubMeshes, and SubMeshes no node
		* references, get a single identity placement. Resolved in Upload();
		* an out-of-range index also yields a single identity placement.
		* Renderers draw each SubMesh with placement * actor world.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1つの SubMesh のモデル空間配置を返す: その SubMesh の meshIndex_
		* を mesh_ で参照する全 Node の globalTransform_。複数ノードから
		* インスタンスされるメッシュはノードごとに1回ずつ描かれる。スキン
		* 付き SubMesh と、どのノードからも参照されない SubMesh は単位行列
		* 1つになる。Upload() で解決し、範囲外のインデックスも単位行列
		* 1つを返す。レンダラーは各 SubMesh を 配置 * アクターワールド で
		* 描く。
		*/
		NODISCARD std::span<const Matrix> SubMeshPlacement(Size subMeshIndex)const;

		/**
		* [EN]
		* Copies the RT proxy's base (bind-pose) positions
		* (positionResource_, VertexCount() * sizeof(Vector3) bytes) into
		* destination, which the caller must have already transitioned to
		* D3D12_RESOURCE_STATE_COPY_DEST. Used to seed a per-instance morph
		* blend scratch buffer before MorphBlendCS overwrites only the
		* vertex ranges of SubMeshes with active morph weights this frame —
		* every other vertex must keep its base position unchanged.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* RT プロキシのベース(バインドポーズ)位置(positionResource_、
		* VertexCount() * sizeof(Vector3) バイト)を destination へコピー
		* する。呼び出し側は destination を事前に
		* D3D12_RESOURCE_STATE_COPY_DEST へ遷移させておくこと。今フレーム
		* 有効なモーフウェイトを持つ SubMesh の頂点範囲だけを MorphBlendCS が
		* 上書きする前の、インスタンスごとのモーフブレンド用一時バッファの
		* 種として使う — それ以外の頂点はベース位置のまま保つ必要がある。
		*/
		void CopyMorph(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination)const;

		/**
		* [EN]
		* Geometry streaming (Nanite-style residency) + budget control
		* overview. A page is one Cluster's slice of meshlets/bounds/vertex-
		* indices/primitive-indices (+ its own vertex range for LOD>=1). LOD 0
		* clusters share the original vertex pool page instead. Pages upload
		* on demand from the CPU-resident arrays and are evicted by
		* EvictClusterBudget when total VRAM exceeds the budget. Pinned pages
		* (coarsest cluster per SubMesh, skinned LOD 0 and the pool they
		* reference) never evict, so every model can always be drawn at some
		* LOD. MakeClusterResident/MakePoolResident/EvictCluster/EvictPool are
		* the resident/evict pair for cluster vs. pool. They are public
		* because ModelRenderer drives MakeClusterResident from outside.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ジオメトリストリーミング（Nanite 型の常駐管理）+ 予算制御の概要。
		* ページは 1 Cluster 分の meshlet/バウンド/頂点インデックス/
		* プリミティブインデックスのスライス（LOD>=1 は専用頂点範囲込み）。
		* LOD 0 クラスタは代わりに元の共有頂点プールページを参照する。
		* ページは CPU 常駐配列からオンデマンドでアップロードされ、VRAM が
		* 予算を超えると EvictClusterBudget が追い出す。ピン留めページ（SubMesh
		* ごとの最粗クラスタ、スキンド LOD 0 とそれが参照するプール）は
		* 追い出さないため、モデルは常に何らかの LOD で描画できる。
		* MakeClusterResident/MakePoolResident/EvictCluster/EvictPool は
		* クラスタ/プールの常駐・追い出しの対。ModelRenderer が外から
		* MakeClusterResident を呼ぶため public にしている。
		*/

		/**
		* [EN]
		* Synchronously uploads the cluster's page (and the vertex pool if
		* the cluster is LOD 0). No-op when already resident.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタのページを同期アップロードする（LOD 0 なら頂点プールも）。
		* 常駐済みなら何もしない。
		*/
		void MakeClusterResident(Uint32 clusterIndex);

		/**
		* [EN]
		* Texture mip counterpart to MakeClusterResident: streams a mip level
		* instead of a cluster (see StreamingTexture below). targetMip is
		* uploaded directly rather than one level at a time, so a texture
		* reaches the resolution the camera wants in a single synchronous
		* upload instead of one fence wait per level.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* MakeClusterResident のテクスチャミップ版の対。クラスタではなく
		* ミップレベルをストリーミングする（下記 StreamingTexture 参照）。
		* targetMip は1段ずつではなく直接アップロードするので、テクスチャは
		* 段ごとにフェンスを待つことなく、1回の同期アップロードでカメラが
		* 求める解像度に届く。
		*/
		void MakeTextureMipResident(Uint32 textureIndex, Uint32 targetMip);

		/**
		* [EN]
		* Synchronously uploads the shared LOD 0 vertex pool page. No-op when
		* already resident, when poolVertexEnd_ is 0 (no LOD 0 clusters), or
		* when no device is bound yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有 LOD 0 頂点プールページを同期アップロードする。常駐済み、
		* poolVertexEnd_ が 0（LOD 0 クラスタが無い）、またはまだ device が
		* 束縛されていない場合は何もしない。
		*/
		void MakePoolResident();

		/**
		* [EN]
		* Frees the cluster page's GPU resources and bindless indices
		* (deferred release, since in-flight frames may still reference
		* them). No-op if not resident or pinned.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページの GPU リソースと bindless インデックスを解放する
		* （インフライトのフレームがまだ参照している可能性があるため遅延
		* 解放）。常駐していないかピン留め済みなら何もしない。
		*/
		void EvictCluster(Uint32 clusterIndex);

		/**
		* [EN]
		* Frees the texture's current (non-pinned) mip resource, falling
		* back to the pinned coarsest mip. No-op if not valid or already at
		* the pinned mip.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* テクスチャの現在の（ピン留めでない）ミップリソースを解放し、
		* ピン留めされた最粗ミップへ戻す。無効、または既にピン留めミップの
		* 場合は何もしない。
		*/
		void EvictTextureMip(Uint32 textureIndex);

		/**
		* [EN]
		* Frees the shared LOD 0 vertex pool page's GPU resource. No-op if
		* not resident, pinned, or still referenced by a resident cluster
		* page (poolResidentReferences_ > 0).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有 LOD 0 頂点プールページの GPU リソースを解放する。常駐していない、
		* ピン留め済み、または常駐中のクラスタページから参照されている場合
		* （poolResidentReferences_ > 0）は何もしない。
		*/
		void EvictPool();

		/**
		* [EN]
		* Marks the cluster page (and its pool, if the cluster doesn't own
		* its own vertices) as used this frame so the eviction age guard
		* keeps it alive while the GPU may still reference it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページ（頂点を自前で持たないクラスタならプールも）を
		* 今フレーム使用中として記録し、GPU がまだ参照しうる間は追い出しの
		* 経過フレームガードで生存させる。
		*/
		void TouchCluster(Uint32 clusterIndex, Uint64 frame);

		/**
		* [EN]
		* Marks the texture's currently-resident mip as used this frame so
		* the eviction age guard keeps it alive while the GPU may still
		* reference it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* テクスチャの現在常駐中のミップを今フレーム使用中として記録し、
		* GPU がまだ参照しうる間は追い出しの経過フレームガードで生存させる。
		*/
		void TouchTexture(Uint32 textureIndex, Uint64 frame);

		/**
		* [EN]
		* Whether the cluster's page is currently GPU-resident.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタのページが現在 GPU に常駐しているか。
		*/
		NODISCARD Bool ClusterResident(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Whether the texture has streamed all the way in to mip 0 (its finest).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* テクスチャがミップ 0（最も細かい）まで完全にストリームインしているか。
		*/
		NODISCARD Bool TextureResident(Uint32 textureIndex)const;

		/**
		* [EN]
		* Evicts unpinned cluster pages (oldest first) across every live
		* Crister, then unpinned/unreferenced vertex pools, until total
		* resident geometry fits geometryBudgetBytes_. Pages must be unused
		* for evictAgeFrames_ frames before eviction so in-flight GPU work
		* never loses a buffer it references.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 生存中の全 Crister を対象に、未ピンのクラスタページを古い順に、
		* 続いて未ピン/未参照の頂点プールを追い出し、常駐ジオメトリ合計を
		* geometryBudgetBytes_ 以内に収める。インフライトの GPU 作業が
		* 参照中のバッファを失わないよう、evictAgeFrames_ フレーム未使用の
		* ページのみ追い出す。
		*/
		static void EvictClusterBudget(Uint64 currentFrame);

		/**
		* [EN]
		* Texture counterpart to EvictClusterBudget: evicts unpinned
		* resident texture mips (oldest first) across every live Crister
		* until total resident texture memory fits textureBudgetBytes_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* EvictClusterBudget のテクスチャ版の対: 生存中の全 Crister を対象に、
		* 未ピンの常駐ミップを古い順に追い出し、常駐テクスチャメモリ合計を
		* textureBudgetBytes_ 以内に収める。
		*/
		static void EvictTextureBudget(Uint64 currentFrame);

		/**
		* [EN]
		* Bindless SRV of the cluster page's vertex buffer — its own page
		* if it owns vertices, otherwise the shared LOD 0 pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページの頂点バッファの bindless SRV — 頂点を自前で持てば
		* 自身のページ、そうでなければ共有 LOD 0 プール。
		*/
		NODISCARD Uint ClusterVertexBufferIndex(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Bindless SRV of the cluster page's meshlet buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページの meshlet バッファの bindless SRV。
		*/
		NODISCARD Uint ClusterMeshletBufferIndex(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Bindless SRV of the cluster page's meshlet bound buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページの meshlet バウンドバッファの bindless SRV。
		*/
		NODISCARD Uint ClusterMeshletBoundBufferIndex(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Bindless SRV of the cluster page's vertex indices buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページの頂点インデックスバッファの bindless SRV。
		*/
		NODISCARD Uint ClusterVertexIndicesBufferIndex(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Bindless SRV of the cluster page's primitive indices buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタページのプリミティブインデックスバッファの bindless SRV。
		*/
		NODISCARD Uint ClusterPrimitiveIndicesBufferIndex(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Whether this cluster page owns its own vertex slice
		* (streamingGeometry_[clusterIndex].ownsVertices_) rather than
		* referencing the shared LOD 0 pool. Own-page vertex indices are
		* rebased to page-local numbering by MakeClusterResident, so they
		* are NOT valid indices into Crister::vertexMorphSource_/
		* morphDeltaResource_ (which use the crister-wide numbering the
		* shared pool preserves) — callers populating a raster morph
		* instance must check this and leave morph fields zeroed
		* (ModelStructuredBuffer::morphTargetCount_ == 0) for any cluster where
		* this returns true.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このクラスタページが共有 LOD 0 プールを参照するのではなく、
		* 自前の頂点スライスを持つか
		* (streamingGeometry_[clusterIndex].ownsVertices_)。自前ページの
		* 頂点インデックスは MakeClusterResident によってページローカルな
		* 番号へリベースされるため、Crister::vertexMorphSource_/
		* morphDeltaResource_(共有プールが保つ Crister 全体の番号付けを使う)
		* への有効なインデックスでは【ない】— ラスタのモーフ用インスタンス
		* を組み立てる側はこれを確認し、true が返るクラスタでは
		* モーフフィールドをゼロのまま
		* (ModelStructuredBuffer::morphTargetCount_ == 0)にすること。
		*/
		NODISCARD Bool StandaloneVertices(Uint32 clusterIndex)const;

		/**
		* [EN]
		* Finest mip level currently GPU-resident, or 0 if textureIndex is
		* out of range.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在 GPU に常駐している最も細かいミップ段。textureIndex が範囲外
		* なら 0。
		*/
		NODISCARD Uint32 TextureFinestMip(Uint32 textureIndex)const;

		/**
		* [EN]
		* Picks the mip a material texture needs so that one texel covers
		* about one screen pixel: texels per world unit (the SubMesh's baked
		* texcoordDensity * texture resolution) versus screen pixels per
		* world unit (worldScale * pixelsPerUnit). texcoordDensity 0 falls
		* back to assuming the texture spans the whole model once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1 テクセルが画面 1 ピクセル程度になるミップを選ぶ: 1 ワールド単位
		* あたりのテクセル数(SubMesh に焼いた texcoordDensity × テクスチャ
		* 解像度)と、1 ワールド単位あたりの画面ピクセル数(worldScale ×
		* pixelsPerUnit)を比べる。texcoordDensity が 0 のときはテクスチャが
		* モデル全体に 1 回貼られている前提にフォールバックする。
		*/
		NODISCARD Uint32 TextureDesiredMip(Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const;

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

		/**
		* [EN]
		* Dequantises just the position field of a CompressedVertex, against
		* this Crister's own positionMin_/positionExtent_. A separate function
		* from DecodeVertex because some callers only need position (deduping
		* by position, or building a position-only buffer) and would otherwise
		* pay for decoding normal/tangent/texcoord they never use.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex の position フィールドだけを、この Crister 自身の
		* positionMin_/positionExtent_ に対して逆量子化する。DecodeVertex とは
		* 別関数にしてあるのは、呼び出し元の一部が position だけを必要とする
		* （position で重複排除する、position だけのバッファを作る）ため —
		* そうしないと使わない normal/tangent/texcoord のデコードまで払うことになる。
		*/
		NODISCARD Vector3 DecodePosition(const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Dequantises just the texcoord field of a CompressedVertex, against
		* this Crister's own texcoordMin_/texcoordExtent_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex の texcoord フィールドだけを、この Crister 自身の
		* texcoordMin_/texcoordExtent_ に対して逆量子化する。
		*/
		NODISCARD Vector2 DecodeTexcoord(const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Dequantises just the tangent field (xyz direction + handedness sign
		* in .w) of a CompressedVertex.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedVertex の tangent フィールド（xyz 方向 + .w の利き手符号）
		* だけを逆量子化する。
		*/
		NODISCARD Vector4 DecodeTangent(const CompressedVertex& compressed)const;

		/**
		* [EN]
		* Dequantises the joint indices and renormalised weights of a
		* CompressedSkinVertex. Weights are renormalised because four 8-bit
		* UNORM values rarely sum to exactly one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CompressedSkinVertex のジョイントインデックスと再正規化済み
		* ウェイトを逆量子化する。4つの 8bit UNORM 値は合計がちょうど1に
		* なることが稀なため再正規化する。
		*/
		void DecodeSkin(const CompressedSkinVertex& compressed, XmUint4& outJoints, Vector4& outWeights)const;

	};
}