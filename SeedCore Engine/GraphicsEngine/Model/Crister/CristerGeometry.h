#pragma once
#include <GraphicsEngine/Model/Cluster/Meshlet.h>

namespace SeedCore
{
	/**
	* [EN]
	* Full-precision CPU-side working vertex format, populated by
	* ModelLoader::FetchMeshes straight from glTF accessors. Only alive
	* during the load/bake pipeline — BakeMesh() quantises this into
	* CompressedVertex/CompressedSkinVertex and discards it, so it is never
	* itself serialised to the .crister cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フル精度の CPU 側ワーキング頂点フォーマット。ModelLoader::FetchMeshes
	* が glTF アクセサから直接埋める。ロード/ベイクのパイプライン中のみ生存
	* — BakeMesh() がこれを CompressedVertex/CompressedSkinVertex へ量子化
	* して破棄するため、これ自体が .crister キャッシュへシリアライズされる
	* ことは無い。
	*/
	struct Vertex
	{
		/// [EN] Object-space position.
		/// [JP] オブジェクト空間の位置。
		Vector3 position_ = { 0,0,0 };

		/// [EN] Object-space unit normal.
		/// [JP] オブジェクト空間の単位法線。
		Vector3 normal_ = { 0,0,1 };

		/// [EN] Tangent in xyz, with w = ±1 giving the handedness of the bitangent.
		/// [JP] xyz が接線、w = ±1 が従接線の向き(利き手)。
		Vector4 tangent_ = { 1,0,0,1 };

		/// [EN] Texture coordinate (TEXCOORD_0).
		/// [JP] テクスチャ座標(TEXCOORD_0)。
		Vector2 texcoord_ = { 0,0 };

		/// [EN] Up to four joint indices into the skin's joint list.
		/// [JP] スキンのジョイント一覧を指す、最大4つのジョイントインデックス。
		XmUint4 joints_ = { 0,0,0,0 };

		/// [EN] Weight of each of joints_; the four sum to 1.
		/// [JP] joints_ のそれぞれの重み。4つの合計は 1。
		Vector4 weights_ = { 1,0,0,0 };
	};

	/**
	* [EN]
	* GPU vertex format, quantised on the CPU at BakeMesh() time (load/bake,
	* baked into the .crister cache) and decoded in the mesh/hit shaders
	* (Model.hlsli: DecodeModelVertex). 16 bytes vs the 80-byte source
	* Vertex. Positions/texcoords are 16-bit UNORM against the whole
	* Crister's AABB (shared vertices quantise identically, so meshlet
	* seams cannot crack); normals are 16+16-bit octahedral; tangents are
	* 8+7-bit octahedral plus a handedness sign.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GPU 頂点フォーマット。BakeMesh() 時（ロード/ベイク時、.crister キャッシュに
	* 焼き込み）に CPU で量子化し、メッシュ/ヒットシェーダでデコードする
	* (Model.hlsli: DecodeModelVertex)。元 Vertex の 80 バイトに対し 16 バイト。
	* 位置/UV は Crister 全体の AABB に対する 16bit UNORM（共有頂点は同一に
	* 量子化されるため、メシュレット境界で亀裂は出ない）。法線は 16+16bit
	* octahedral、タンジェントは 8+7bit octahedral + 利き手符号 1bit。
	*/
	struct CompressedVertex
	{
		/// [EN] Position x:16 | y:16, UNORM within the Crister's AABB.
		/// [JP] 位置 x:16 | y:16。Crister の AABB 内の UNORM。
		Uint32 positionXY_ = 0;

		/// [EN] Position z:16 (UNORM within the AABB) | texcoord u:16 (UNORM within the UV AABB).
		/// [JP] 位置 z:16(AABB 内の UNORM) | テクスチャ座標 u:16(UV の AABB 内の UNORM)。
		Uint32 positionZTexU_ = 0;

		/// [EN] Texcoord v:16 | octahedral tangent x:8 y:7 | handedness sign:1.
		/// [JP] テクスチャ座標 v:16 | octahedral の接線 x:8 y:7 | 利き手の符号:1。
		Uint32 texVTangent_ = 0;

		/// [EN] Octahedral normal x:16 | y:16.
		/// [JP] octahedral の法線 x:16 | y:16。
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

	/**
	* [EN]
	* Skinning attributes, split from CompressedVertex so static models
	* never pay for them. Uploaded only when the Crister has skins.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スキニング属性。静的モデルがコストを払わずに済むよう
	* CompressedVertex から分離。スキンを持つ Crister のみアップロード。
	*/
	struct CompressedSkinVertex
	{
		/// [EN] Joint indices joint0:16 | joint1:16.
		/// [JP] ジョイントインデックス joint0:16 | joint1:16。
		Uint32 jointsXY_ = 0;

		/// [EN] Joint indices joint2:16 | joint3:16.
		/// [JP] ジョイントインデックス joint2:16 | joint3:16。
		Uint32 jointsZW_ = 0;

		/// [EN] Four 8-bit UNORM weights, renormalised in the shader so they sum to 1 again.
		/// [JP] 8bit UNORM の重み4つ。合計が 1 に戻るようシェーダーで正規化し直す。
		Uint32 weights_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("joints_xy", jointsXY_);
			archive.Field("joints_zw", jointsZW_);
			archive.Field("weights", weights_);
		}
	};

	/**
	* [EN]
	* One LOD level's meshlet range within a SubMesh: meshletOffset_/
	* meshletCount_ index into meshlets_/meshletBounds_, lodLevel_ selects
	* the detail tier (0 = most detailed), and lodError_ is the QEM
	* simplification error used to pick which Cluster to draw for a given
	* screen coverage. This is also the streaming granularity — see
	* StreamingGeometry, MakeClusterResident/EvictCluster.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SubMesh 内の 1 LOD レベルぶんの meshlet 範囲: meshletOffset_/
	* meshletCount_ で meshlets_/meshletBounds_ を指す。lodLevel_ が
	* 詳細度の段（0 が最も詳細）、lodError_ は QEM 簡略化誤差で、画面
	* 被覆率に応じてどの Cluster を描画するか選ぶのに使う。これは
	* ストリーミングの粒度でもある — StreamingGeometry、
	* MakeClusterResident/EvictCluster 参照。
	*/
	struct Cluster
	{
		/// [EN] First meshlet of this LOD in meshlets_/meshletBounds_.
		/// [JP] この LOD の最初の meshlet の、meshlets_/meshletBounds_ での位置。
		Uint32 meshletOffset_ = 0;

		/// [EN] Number of meshlets in this LOD.
		/// [JP] この LOD の meshlet の数。
		Uint32 meshletCount_ = 0;

		/// [EN] Detail tier; 0 is the most detailed.
		/// [JP] 詳細度の段。0 が最も詳細。
		Uint32 lodLevel_ = 0;

		/// [EN] QEM simplification error of this LOD, compared against screen coverage to choose which Cluster to draw.
		/// [JP] この LOD の QEM 簡略化誤差。画面被覆率と比べて、どの Cluster を描画するかを選ぶ。
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

	/**
	* [EN]
	* One glTF morph target belonging to a SubMesh: a display name plus
	* per-vertex position deltas, one entry per vertex in that SubMesh (index
	* i is the delta for the vertex at crister.vertices_[vertexOffset + i]
	* during load, and stays index-aligned with the SubMesh's own vertex
	* range afterwards). Kept as full-precision Vector3 rather than folded
	* into CompressedVertex's shared-AABB quantisation: a morph delta's
	* magnitude is unrelated to the mesh's own bounds and quantising it
	* against them would misrepresent targets that move vertices far
	* outside the rest pose.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SubMesh に属する glTF モーフターゲット1つぶん: 表示名と、頂点ごとの
	* 位置デルタ（そのSubMeshの頂点1つにつき1エントリ。ロード時はインデックス i が
	* crister.vertices_[vertexOffset + i] のデルタに対応し、その後もSubMesh自身の
	* 頂点範囲とインデックスが揃ったまま保たれる）。CompressedVertex の
	* 共有AABB量子化には畳み込まずフル精度のVector3で保持する — モーフデルタの
	* 大きさはメッシュ自身の境界とは無関係で、それに対して量子化すると
	* レストポーズから大きく外れて動くターゲットを正しく表現できない。
	*/
	struct Morph
	{
		/// [EN] Display name of the target as the source file names it, or "Morph_<index>" when it has none.
		/// [JP] 元ファイルが付けているターゲットの表示名。名前が無ければ "Morph_<index>"。
		std::string name_;

		/// [EN] Position delta of each vertex of the SubMesh at full weight, index-aligned with its vertex range.
		/// [JP] 重み 1 のときの、SubMesh の各頂点の位置デルタ。SubMesh の頂点範囲とインデックスが揃っている。
		DynamicArray<Vector3> positionDeltas_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("position_deltas", positionDeltas_);
		}
	};

	/**
	* [EN]
	* One drawable piece of the model: a single material assignment plus
	* clusterOffset_/clusterCount_ (all its LOD Clusters) and
	* indexOffset_/indexCount_ (its range in the flat 32-bit triangle
	* index buffer, for RT). Every glTF primitive becomes one SubMesh.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルの描画可能な1パーツ: 単一のマテリアル割り当てと
	* clusterOffset_/clusterCount_（全 LOD の Cluster）、
	* indexOffset_/indexCount_（フラット 32bit 三角形インデックスバッファ
	* 内の範囲、RT 用）。glTF の各プリミティブが1つの SubMesh になる。
	*/
	struct SubMesh
	{
		/// [EN] First of this SubMesh's LOD Clusters in clusters_.
		/// [JP] この SubMesh の LOD の Cluster のうち、最初のものの clusters_ での位置。
		Uint32 clusterOffset_ = 0;

		/// [EN] Number of LOD Clusters this SubMesh has.
		/// [JP] この SubMesh が持つ LOD の Cluster の数。
		Uint32 clusterCount_ = 0;

		/// [EN] Index into surfaces_ of the material this SubMesh is drawn with.
		/// [JP] この SubMesh を描画するマテリアルの、surfaces_ でのインデックス。
		Uint32 surfaceIndex_ = 0;

		/// [EN] First index of this SubMesh in the flat 32-bit triangle index buffer.
		/// [JP] フラットな 32bit 三角形インデックスバッファでの、この SubMesh の最初のインデックス。
		Uint32 indexOffset_ = 0;

		/// [EN] Number of indices this SubMesh has in that buffer.
		/// [JP] そのバッファでこの SubMesh が持つインデックスの数。
		Uint32 indexCount_ = 0;

		/// [EN] Index into Crister::skins_ if this SubMesh is skinned, -1 otherwise. Resolved from the glTF node that references this SubMesh's mesh.
		/// [JP] この SubMesh がスキンドなら Crister::skins_ へのインデックス、それ以外は -1。この SubMesh のメッシュを参照する glTF ノードから解決される。
		Int skinIndex_ = -1;

		/// [EN] Index of the source mesh this SubMesh came from; SubMeshes from the same mesh share it, and nodes refer to them through it.
		/// [JP] この SubMesh の元になったメッシュのインデックス。同じメッシュから来た SubMesh は同じ値を持ち、ノードはこれを通して参照する。
		Int meshIndex_ = -1;

		/// [EN] Empty for a SubMesh with no glTF morph targets.
		/// [JP] glTF モーフターゲットを持たない SubMesh では空。
		DynamicArray<Morph> morphs_;

		/// [EN] First vertex of this SubMesh in vertices_.
		/// [JP] この SubMesh の最初の頂点の、vertices_ での位置。
		Uint32 vertexOffset_ = 0;

		/// [EN] Number of vertices this SubMesh has.
		/// [JP] この SubMesh が持つ頂点の数。
		Uint32 vertexCount_ = 0;

		/// [EN] First vertex of this SubMesh in the RT proxy vertex buffer.
		/// [JP] RT プロキシの頂点バッファでの、この SubMesh の最初の頂点。
		Uint32 raytracingVertexOffset_ = 0;

		/// [EN] Number of vertices this SubMesh has in the RT proxy, covering every node placement of it.
		/// [JP] RT プロキシでこの SubMesh が持つ頂点の数。この SubMesh の全てのノード配置ぶんを含む。
		Uint32 raytracingVertexCount_ = 0;

		/// [EN] First triangle of this SubMesh in the RT proxy.
		/// [JP] RT プロキシでの、この SubMesh の最初の三角形。
		Uint32 raytracingTriangleOffset_ = 0;

		/// [EN] Number of triangles this SubMesh has in the RT proxy.
		/// [JP] RT プロキシでこの SubMesh が持つ三角形の数。
		Uint32 raytracingTriangleCount_ = 0;

		/// [EN] First delta of this SubMesh in the RT proxy's morph delta pool, laid out per target; SC_INVALID when it has no morphs.
		/// [JP] RT プロキシのモーフデルタプールでの、この SubMesh の最初のデルタ。ターゲットごとに並ぶ。モーフが無ければ SC_INVALID。
		Uint32 raytracingMorphDeltaOffset_ = SC_INVALID;

		/// [EN] First delta of this SubMesh in the raster morph delta pool, vertexCount_ deltas per target; SC_INVALID when it has no morphs.
		/// [JP] ラスタのモーフデルタプールでの、この SubMesh の最初のデルタ。ターゲットごとに vertexCount_ 個ずつ並ぶ。モーフが無ければ SC_INVALID。
		Uint32 morphDeltaOffset_ = SC_INVALID;

		/// [EN] UV distance per unit of surface distance, the square root of UV area over surface area; times the texture size it gives texels per unit, which picks the mip to stream for the on-screen size. 0 when either area is degenerate.
		/// [JP] 表面上の距離 1 あたりの UV 上の距離。UV 面積を表面積で割った値の平方根。テクスチャサイズを掛けると 1 単位あたりのテクセル数になり、画面上の大きさに対してストリーミングするミップを決める。どちらかの面積が潰れていれば 0。
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

	/**
	* [EN]
	* Which cluster BakeCollision pulls from a SubMesh: the coarsest
	* cluster or the LOD 0 cluster.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* BakeCollision が SubMesh のどのクラスタから抽出するか:
	* 最粗クラスタか LOD 0 クラスタか。
	*/
	enum class MeshCollisionDetail
	{
		/// [EN] Each SubMesh's coarsest cluster, the cheapest shape to collide against.
		/// [JP] 各 SubMesh の最粗クラスタ。衝突判定が最も軽い形。
		Proxy,

		/// [EN] Each SubMesh's LOD 0 (most detailed) cluster.
		/// [JP] 各 SubMesh の LOD 0（最も詳細な）クラスタ。
		Exact,
	};

	/**
	* [EN]
	* Geometry of a Crister: the quantised vertices and skinning
	* attributes, the meshlet and LOD cluster hierarchy, the SubMeshes,
	* and the AABBs the vertices are quantised against. Filled by
	* ModelLoader and the bake classes, or read from a .crister cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister のジオメトリ。量子化済みの頂点とスキニング属性、meshlet と
	* LOD クラスタの階層、SubMesh、頂点の量子化に使う AABB を持つ。
	* ModelLoader と焼き込みのクラスが埋めるか、.crister キャッシュから
	* 読み込む。
	*/
	struct CristerGeometry
	{
		/// [EN] Full-precision source vertices, alive only during the load/bake pipeline (FetchMeshes -> BuildMeshlets -> BakeMesh) and not serialized; BakeMesh() turns them into compressedVertices_/compressedSkinVertices_ and clears them.
		/// [JP] フル精度のソース頂点。ロード/ベイクのパイプライン(FetchMeshes -> BuildMeshlets -> BakeMesh)の間だけ存在し、シリアライズしない。BakeMesh() がこれを compressedVertices_/compressedSkinVertices_ へ変換して空にする。
		DynamicArray<Vertex> vertices_;

		/// [EN] For each vertex in vertices_/compressedVertices_, including the LOD copies BuildMeshlets appends, the original (pre-LOD) vertex it descends from: itself for an original vertex, otherwise the vertex the QEM step copied its attributes from. Meaningful only within a SubMesh's own range. The raster morph blend (the model mesh shaders, Model/Material/MaterialResolveCS.hlsl and Model/Transparent/ModelTransparentPS.hlsl) uses it to map a vertex of any streamed LOD back to the vertex Morph::positionDeltas_ is indexed by; the RT proxy is built from LOD 0 and needs no such map. It is serialized and kept by BakeMesh(), since it stays index-aligned with compressedVertices_.
		/// [JP] vertices_/compressedVertices_ の各頂点(BuildMeshlets が追加する LOD のコピーも含む)について、元になった(LOD 適用前の)頂点。オリジナルの頂点なら自分自身、それ以外は QEM の段階で属性をコピーした元の頂点。SubMesh 自身の範囲内でのみ意味を持つ。ラスタのモーフブレンド(モデル用メッシュシェーダー、Model/Material/MaterialResolveCS.hlsl、Model/Transparent/ModelTransparentPS.hlsl)が、ストリーミングされたどの LOD の頂点も Morph::positionDeltas_ のインデックスの頂点へ戻すために使う。RT プロキシは LOD 0 から作るので、この対応は要らない。compressedVertices_ とインデックスが揃ったままである必要があるので、シリアライズし、BakeMesh() でも消さない。
		DynamicArray<Uint32> vertexMorphSource_;

		/// [EN] Quantised GPU vertex format, baked by BakeMesh() and cached to disk. Indexed the same way vertices_ was (global vertex index).
		/// [JP] 量子化済み GPU 頂点フォーマット。BakeMesh() で焼き込みディスクにキャッシュする。vertices_ と同じグローバル頂点インデックスで引く。
		DynamicArray<CompressedVertex> compressedVertices_;

		/// [EN] Quantised skinning attributes, baked alongside compressedVertices_. Empty when skins_ is empty.
		/// [JP] 量子化済みスキニング属性。compressedVertices_ と一緒に焼き込む。skins_ が空なら空のまま。
		DynamicArray<CompressedSkinVertex> compressedSkinVertices_;

		/// [EN] Global vertex index of each meshlet-local vertex, meshlet after meshlet (MeshletDesc::vertexOffset_ points in here).
		/// [JP] meshlet ローカルな各頂点の、グローバル頂点インデックス。meshlet ごとに続けて並ぶ(MeshletDesc::vertexOffset_ がここを指す)。
		DynamicArray<Uint32> vertexIndices_;

		/// [EN] Three meshlet-local vertex numbers per triangle, one byte each (MeshletDesc::triangleOffset_ points in here).
		/// [JP] 三角形ごとに meshlet ローカルな頂点番号を3つ、1バイトずつ(MeshletDesc::triangleOffset_ がここを指す)。
		DynamicArray<Uint8> primitiveIndices_;

		/// [EN] Every meshlet of every LOD of every SubMesh; each Cluster owns a contiguous range.
		/// [JP] 全 SubMesh の全 LOD の全 meshlet。各 Cluster が連続した範囲を持つ。
		DynamicArray<MeshletDesc> meshlets_;

		/// [EN] Culling bounds of each meshlet, index-aligned with meshlets_.
		/// [JP] 各 meshlet のカリング用の境界。meshlets_ とインデックスが揃っている。
		DynamicArray<MeshletBound> meshletBounds_;

		/// [EN] Every LOD Cluster of every SubMesh; each SubMesh owns a contiguous range, LOD 0 first.
		/// [JP] 全 SubMesh の全 LOD の Cluster。各 SubMesh が連続した範囲を持ち、LOD 0 から並ぶ。
		DynamicArray<Cluster> clusters_;

		/// [EN] Drawable pieces of the model, one per source primitive.
		/// [JP] モデルの描画可能なパーツ。元のプリミティブ1つにつき1つ。
		DynamicArray<SubMesh> subMeshes_;

		/// [EN] Minimum corner of the position dequantisation AABB, computed by BakeMesh() and serialized.
		/// [JP] 位置の逆量子化 AABB の最小コーナー。BakeMesh() で計算し、シリアライズする。
		Vector3 positionMin_ = { 0,0,0 };

		/// [EN] Extent of the position dequantisation AABB, clamped away from zero on every axis.
		/// [JP] 位置の逆量子化 AABB の大きさ。どの軸も 0 から離してクランプしている。
		Vector3 positionExtent_ = { 1,1,1 };

		/// [EN] Minimum corner of the texcoord dequantisation AABB, computed by BakeMesh() and serialized.
		/// [JP] テクスチャ座標の逆量子化 AABB の最小コーナー。BakeMesh() で計算し、シリアライズする。
		Vector2 texcoordMin_ = { 0,0 };

		/// [EN] Extent of the texcoord dequantisation AABB, clamped away from zero on every axis.
		/// [JP] テクスチャ座標の逆量子化 AABB の大きさ。どの軸も 0 から離してクランプしている。
		Vector2 texcoordExtent_ = { 1,1 };
	};
}