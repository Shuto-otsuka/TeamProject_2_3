#pragma once
#include <GraphicsEngine/Model/Cluster/Meshlet.h>

/**
* [EN]
* Crister streams geometry and texture mips the same way: the coarsest
* level stays pinned resident, finer levels are made resident on demand,
* and EvictClusterBudget/EvictTextureBudget reclaim them under separate
* VRAM budgets (see MakeClusterResident/EvictCluster and
* MakeTextureMipResident/EvictTextureMip). Each texture mip level is its
* own committed resource, created and freed independently like
* StreamingGeometry's per-cluster buffers, because a DEFAULT-heap
* resource reserves memory for its whole mip chain at creation no matter
* which subresources hold data. UV space does not depend on which
* single-mip resource backs the SRV, so the shaders sample whatever
* resolution is resident without knowing about streaming.
*
* ---------------------------------------------------------------------
*
* [JP]
* Crister はジオメトリとテクスチャのミップを同じ形でストリーミングする。
* 最も粗いレベルはピン留めして常駐させ、細かいレベルは必要に応じて
* 常駐させ、EvictClusterBudget/EvictTextureBudget がそれぞれ別の VRAM
* 予算で回収する（MakeClusterResident/EvictCluster と
* MakeTextureMipResident/EvictTextureMip 参照）。テクスチャのミップは
* レベルごとに別々の committed resource で、StreamingGeometry のクラスタ
* ごとのバッファと同じく個別に作成・破棄する。DEFAULT ヒープのリソースは、
* どのサブリソースにデータがあるかに関わらず作成時点でミップチェーン全体の
* メモリを確保するためである。UV 空間は SRV の裏にある単一ミップのリソース
* によらないので、シェーダーはストリーミングを意識せず、常駐中の解像度を
* そのままサンプリングする。
*/
namespace SeedCore
{
	class BindlessHeap;
	class BC7CompressShader;
	class D3D12CommandQueue;

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
	* One glTF scene: a name plus the list of its root-level node
	* indices into nodes_. defaultStage_ selects which Stage renders by
	* default when none is explicitly chosen.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の1シーンぶん: 名前と、nodes_ を指すルートレベルのノード
	* インデックス一覧。defaultStage_ が、明示的な選択が無い時に
	* デフォルトで描画される Stage を選ぶ。
	*/
	struct Stage
	{
		/// [EN] Name of the glTF scene.
		/// [JP] glTF シーンの名前。
		std::string name_;

		/// [EN] Indices into nodes_ of the scene's root-level nodes.
		/// [JP] シーンのルートレベルのノードの、nodes_ でのインデックス。
		DynamicArray<Int> nodes_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("nodes", nodes_);
		}
	};

	/**
	* [EN]
	* Bundles every glTF KHR_materials_* extension a Material can carry.
	* Each extension's default is set so that "extension absent from the
	* glTF" and "extension present with default values" behave identically
	* (neutral / no effect).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の KHR_materials_* 拡張をまとめて持つ。各拡張のデフォルトは
	* 「拡張が glTF に無い時＝中立（効果なし）」になるよう設定している。
	*/
	struct KHR
	{
		/**
		* [EN]
		* KHR_materials_emissive_strength: emissive intensity multiplier
		* (>1 for HDR emission). Default 1.0 = neutral.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_emissive_strength: エミッシブ強度（>1 で HDR 発光）。既定 1.0＝中立。
		*/
		struct EmissiveStrength
		{
			/// [EN] Multiplier on the emissive color; values above 1 give HDR emission.
			/// [JP] エミッシブ色に掛ける倍率。1 を超えると HDR の発光になる。
			Float emissiveStrength_ = 1.0f;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("emissive_strength", emissiveStrength_);
			}
		};

		/// [EN] KHR_materials_emissive_strength.
		/// [JP] KHR_materials_emissive_strength。
		EmissiveStrength emissiveStrength_;

		/**
		* [EN]
		* KHR_materials_ior: dielectric index of refraction. Default 1.5
		* (glTF's own default). Feeds the F0 (Fresnel reflectance at normal
		* incidence) computation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_ior: 誘電体の屈折率。既定 1.5（glTF 既定）。F0 計算に効く。
		*/
		struct Ior
		{
			/// [EN] Index of refraction of the dielectric; F0 is derived from it.
			/// [JP] 誘電体の屈折率。F0 はこれから求める。
			Float ior_ = 1.5f;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("ior", ior_);
			}
		};

		/// [EN] KHR_materials_ior.
		/// [JP] KHR_materials_ior。
		Ior ior_;

		/**
		* [EN]
		* KHR_materials_specular: specular intensity and tint. Default
		* factor=1 / color=white = full specular (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_specular: スペキュラ強度と色。既定 factor=1/color=白＝フルスペキュラ（中立）。
		*/
		struct Specular
		{
			/// [EN] Strength of the specular reflection.
			/// [JP] スペキュラ反射の強さ。
			Float specularFactor_ = 1.0f;

			/// [EN] Linear RGB tint of the specular reflection at normal incidence.
			/// [JP] 垂直入射でのスペキュラ反射の色味(リニア RGB)。
			Float specularColorFactor_[3] = { 1,1,1 };

			/// [EN] Image in bitmaps_ whose alpha scales specularFactor_; SC_INVALID when absent.
			/// [JP] アルファで specularFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 specularTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose RGB scales specularColorFactor_; SC_INVALID when absent.
			/// [JP] RGB で specularColorFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 specularColorTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("specular_factor", specularFactor_);
				archive.TryField("specular_color_factor", specularColorFactor_);
				archive.TryField("specular_texture_index", specularTextureIndex_);
				archive.TryField("specular_color_texture_index", specularColorTextureIndex_);
			}
		};

		/// [EN] KHR_materials_specular.
		/// [JP] KHR_materials_specular。
		Specular specular_;

		/**
		* [EN]
		* KHR_materials_clearcoat: an extra clear-coat lobe. Default
		* factor=0 = no coat (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_clearcoat: クリアコート層。既定 factor=0＝コート無し（中立）。
		*/
		struct ClearCoat
		{
			/// [EN] Strength of the clear-coat layer; 0 means no coat.
			/// [JP] クリアコート層の強さ。0 ならコート無し。
			Float clearCoatFactor_ = 0.0f;

			/// [EN] Roughness of the clear-coat layer.
			/// [JP] クリアコート層のラフネス。
			Float clearCoatRoughnessFactor_ = 0.0f;

			/// [EN] Image in bitmaps_ whose red channel scales clearCoatFactor_; SC_INVALID when absent.
			/// [JP] R チャンネルで clearCoatFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 clearCoatTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose green channel scales clearCoatRoughnessFactor_; SC_INVALID when absent.
			/// [JP] G チャンネルで clearCoatRoughnessFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 clearCoatRoughnessTextureIndex_ = SC_INVALID;

			/// [EN] Normal map of the clear-coat layer in bitmaps_; SC_INVALID when absent.
			/// [JP] クリアコート層の法線マップ(bitmaps_ の画像)。無ければ SC_INVALID。
			Uint32 clearCoatNormalTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("clear_coat_factor", clearCoatFactor_);
				archive.TryField("clear_coat_roughness_factor", clearCoatRoughnessFactor_);
				archive.TryField("clear_coat_texture_index", clearCoatTextureIndex_);
				archive.TryField("clear_coat_roughness_texture_index", clearCoatRoughnessTextureIndex_);
				archive.TryField("clear_coat_normal_texture_index", clearCoatNormalTextureIndex_);
			}
		};

		/// [EN] KHR_materials_clearcoat.
		/// [JP] KHR_materials_clearcoat。
		ClearCoat clearCoat_;

		/**
		* [EN]
		* KHR_materials_transmission: thin-glass transmission. Default
		* factor=0 = opaque (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_transmission: 透過（薄いガラス）。既定 factor=0＝不透過（中立）。
		*/
		struct Transmission
		{
			/// [EN] Fraction of light transmitted through the surface; 0 is opaque.
			/// [JP] 表面を透過する光の割合。0 なら不透過。
			Float transmissionFactor_ = 0.0f;

			/// [EN] Image in bitmaps_ whose red channel scales transmissionFactor_; SC_INVALID when absent.
			/// [JP] R チャンネルで transmissionFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 transmissionTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("transmission_factor", transmissionFactor_);
				archive.TryField("transmission_texture_index", transmissionTextureIndex_);
			}
		};

		/// [EN] KHR_materials_transmission.
		/// [JP] KHR_materials_transmission。
		Transmission transmission_;

		/**
		* [EN]
		* KHR_materials_volume: internal volume (thickness + absorption),
		* used together with Transmission. Default thickness=0 /
		* attenuationDistance=infinity (no absorption) / color=white =
		* neutral.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_volume: 内部ボリューム（厚み・吸収）。transmission と併用。
		* 既定 thickness=0/attenuationDistance=∞（吸収なし）/color=白＝中立。
		*/
		struct Volume
		{
			/// [EN] Thickness of the volume under the surface, in model units; 0 makes it thin-walled.
			/// [JP] 表面の下にあるボリュームの厚み(モデル空間の単位)。0 なら薄い壁として扱う。
			Float thicknessFactor_ = 0.0f;

			/// [EN] Distance light travels in the medium before reaching attenuationColor_; FLT_MAX means no absorption.
			/// [JP] 媒質内で光が attenuationColor_ になるまでに進む距離。FLT_MAX なら吸収しない。
			Float attenuationDistance_ = FLT_MAX;

			/// [EN] Linear RGB color white light becomes after attenuationDistance_ in the medium.
			/// [JP] 白色光が媒質内を attenuationDistance_ 進んだ後になる色(リニア RGB)。
			Float attenuationColor_[3] = { 1,1,1 };

			/// [EN] Image in bitmaps_ whose green channel scales thicknessFactor_; SC_INVALID when absent.
			/// [JP] G チャンネルで thicknessFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 thicknessTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("thickness_factor", thicknessFactor_);
				archive.TryField("attenuation_distance", attenuationDistance_);
				archive.TryField("attenuation_color", attenuationColor_);
				archive.TryField("thickness_texture_index", thicknessTextureIndex_);
			}
		};

		/// [EN] KHR_materials_volume.
		/// [JP] KHR_materials_volume。
		Volume volume_;

		/**
		* [EN]
		* KHR_materials_sheen: fabric-like sheen lobe. Default color=black =
		* no sheen (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_sheen: 布の光沢。既定 color=黒＝シーン無し（中立）。
		*/
		struct Sheen
		{
			/// [EN] Linear RGB color of the sheen; black means no sheen.
			/// [JP] シーンの色(リニア RGB)。黒ならシーン無し。
			Float sheenColorFactor_[3] = { 0,0,0 };

			/// [EN] Roughness of the sheen lobe.
			/// [JP] シーンのローブのラフネス。
			Float sheenRoughnessFactor_ = 0.0f;

			/// [EN] Image in bitmaps_ whose RGB scales sheenColorFactor_; SC_INVALID when absent.
			/// [JP] RGB で sheenColorFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 sheenColorTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose alpha scales sheenRoughnessFactor_; SC_INVALID when absent.
			/// [JP] アルファで sheenRoughnessFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 sheenRoughnessTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("sheen_color_factor", sheenColorFactor_);
				archive.TryField("sheen_roughness_factor", sheenRoughnessFactor_);
				archive.TryField("sheen_color_texture_index", sheenColorTextureIndex_);
				archive.TryField("sheen_roughness_texture_index", sheenRoughnessTextureIndex_);
			}
		};

		/// [EN] KHR_materials_sheen.
		/// [JP] KHR_materials_sheen。
		Sheen sheen_;

		/**
		* [EN]
		* KHR_materials_iridescence: thin-film iridescence (soap bubble /
		* oil slick). Default factor=0 = none (neutral). Thickness is in
		* nanometres (default min=100/max=400), ior default 1.3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_iridescence: 虹色（シャボン玉・油膜）。既定 factor=0＝無し（中立）。
		* 厚みは nm 単位（既定 min=100/max=400）、ior 既定 1.3。
		*/
		struct Iridescence
		{
			/// [EN] Strength of the thin-film iridescence; 0 means none.
			/// [JP] 薄膜干渉による虹色の強さ。0 なら無し。
			Float iridescenceFactor_ = 0.0f;

			/// [EN] Index of refraction of the thin film.
			/// [JP] 薄膜の屈折率。
			Float iridescenceIor_ = 1.3f;

			/// [EN] Film thickness in nanometres where the thickness texture reads 0.
			/// [JP] 厚みテクスチャが 0 の位置での膜厚(nm)。
			Float iridescenceThicknessMinimum_ = 100.0f;

			/// [EN] Film thickness in nanometres where the thickness texture reads 1, or everywhere when there is no texture.
			/// [JP] 厚みテクスチャが 1 の位置での膜厚(nm)。テクスチャが無ければ全体がこの厚み。
			Float iridescenceThicknessMaximum_ = 400.0f;

			/// [EN] Image in bitmaps_ whose red channel scales iridescenceFactor_; SC_INVALID when absent.
			/// [JP] R チャンネルで iridescenceFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 iridescenceTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose green channel blends between the minimum and maximum thickness; SC_INVALID when absent.
			/// [JP] G チャンネルで最小と最大の膜厚の間を補間する、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 iridescenceThicknessTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("iridescence_factor", iridescenceFactor_);
				archive.TryField("iridescence_ior", iridescenceIor_);
				archive.TryField("iridescence_thickness_minimum", iridescenceThicknessMinimum_);
				archive.TryField("iridescence_thickness_maximum", iridescenceThicknessMaximum_);
				archive.TryField("iridescence_texture_index", iridescenceTextureIndex_);
				archive.TryField("iridescence_thickness_texture_index", iridescenceThicknessTextureIndex_);
			}
		};

		/// [EN] KHR_materials_iridescence.
		/// [JP] KHR_materials_iridescence。
		Iridescence iridescence_;

		/**
		* [EN]
		* KHR_materials_anisotropy: anisotropic specular (brushed metal,
		* hair). Default strength=0 = isotropic (neutral). rotation is in
		* radians.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_anisotropy: 異方性スペキュラ（ヘアライン金属・髪）。
		* 既定 strength=0＝等方（中立）。rotation はラジアン。
		*/
		struct Anisotropy
		{
			/// [EN] Strength of the anisotropy; 0 is isotropic.
			/// [JP] 異方性の強さ。0 なら等方。
			Float anisotropyStrength_ = 0.0f;

			/// [EN] Rotation of the anisotropy direction from the tangent, in radians.
			/// [JP] 接線から測った異方性の向きの回転(ラジアン)。
			Float anisotropyRotation_ = 0.0f;

			/// [EN] Image in bitmaps_ holding the direction (red/green) and strength (blue) of the anisotropy; SC_INVALID when absent.
			/// [JP] 異方性の向き(R/G)と強さ(B)を持つ、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 anisotropyTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("anisotropy_strength", anisotropyStrength_);
				archive.TryField("anisotropy_rotation", anisotropyRotation_);
				archive.TryField("anisotropy_texture_index", anisotropyTextureIndex_);
			}
		};

		/// [EN] KHR_materials_anisotropy.
		/// [JP] KHR_materials_anisotropy。
		Anisotropy anisotropy_;

		/**
		* [EN]
		* KHR_materials_unlit: disables lighting (flat shading). 1 when the
		* extension is present, default 0 = normal lighting.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_unlit: ライティング無効（フラット）。存在で 1、既定 0＝通常ライティング。
		*/
		struct Unlit
		{
			/// [EN] 1 when the material is unlit, 0 for normal lighting.
			/// [JP] ライティング無しのマテリアルなら 1、通常のライティングなら 0。
			Int unlit_ = 0;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("unlit", unlit_);
			}
		};

		/// [EN] KHR_materials_unlit.
		/// [JP] KHR_materials_unlit。
		Unlit unlit_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("emissive_strength", emissiveStrength_);
			archive.TryField("ior", ior_);
			archive.TryField("specular", specular_);
			archive.TryField("clear_coat", clearCoat_);
			archive.TryField("transmission", transmission_);
			archive.TryField("volume", volume_);
			archive.TryField("sheen", sheen_);
			archive.TryField("iridescence", iridescence_);
			archive.TryField("anisotropy", anisotropy_);
			archive.TryField("unlit", unlit_);
		}
	};

	/**
	* [EN]
	* Which lighting response DeferredLightingPS.hlsl evaluates a material's
	* direct/indirect light with. Each model is not a separate shader or
	* PSO: the deferred lighting pass reads this per instance and switches
	* within one shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DeferredLightingPS.hlsl がマテリアルの直接光/間接光をどのライティング
	* 応答で評価するか。モデルごとに別のシェーダーや PSO があるのではなく、
	* デフォードライティングパスがインスタンスごとにこれを読み、1つの
	* シェーダーの中で切り替える。
	*/
	enum class ShadingModel :Uint32
	{
		/// [EN] Physically based metallic-roughness shading.
		/// [JP] 物理ベースの metallic-roughness シェーディング。
		Pbr = 0,

		/// [EN] No lighting; the base color is shown as is.
		/// [JP] ライティング無し。ベースカラーをそのまま表示する。
		Unlit = 1,

		/// [EN] Blinn-Phong diffuse and specular.
		/// [JP] Blinn-Phong の拡散とスペキュラ。
		Phong = 2,

		/// [EN] Banded cel shading.
		/// [JP] 段階的に塗り分けるセルシェーディング。
		Toon = 3,

		/// [EN] Lambert diffuse only.
		/// [JP] Lambert の拡散のみ。
		Lambert = 4,

		/// [EN] Faceted shading from the face normal.
		/// [JP] 面法線による面ごとのシェーディング。
		Flat = 5,

		/// [EN] Fur drawn as stacked shells (see Surface::furLength_ and its siblings).
		/// [JP] 重ねたシェルで描く毛(Surface::furLength_ などを参照)。
		Fur = 6,
	};

	/**
	* [EN]
	* glTF PBR metallic-roughness material: base factors, alpha mode/
	* cutoff/double-sidedness, bundled KHR_materials_* extensions (khr_),
	* and bindless-heap-agnostic texture indices (resolved into actual
	* bindless indices at Upload() time via TextureBindlessIndex).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の PBR metallic-roughness マテリアル: 基本ファクタ、アルファ
	* モード/カットオフ/両面描画フラグ、まとめた KHR_materials_* 拡張
	* (khr_)、そして bindless ヒープに依存しないテクスチャインデックス
	* （Upload() 時に TextureBindlessIndex 経由で実際の bindless
	* インデックスへ解決される）。
	*/
	struct Surface
	{
		/// [EN] Material name from the source file.
		/// [JP] 元ファイルでのマテリアル名。
		std::string name_;

		/// [EN] Linear RGBA base color factor, multiplied with the base color texture.
		/// [JP] ベースカラーの係数(リニア RGBA)。ベースカラーテクスチャに掛ける。
		Color baseColor_ = { 1,1,1,1 };

		/// [EN] Metalness factor, multiplied with the texture's blue channel.
		/// [JP] メタリックの係数。テクスチャの B チャンネルに掛ける。
		Float metallic_ = 0.0f;

		/// [EN] Perceptual roughness factor, multiplied with the texture's green channel.
		/// [JP] 知覚的ラフネスの係数。テクスチャの G チャンネルに掛ける。
		Float roughness_ = 1.0f;

		/// [EN] Linear RGB emissive factor, multiplied with the emissive texture and khr_.emissiveStrength_.
		/// [JP] エミッシブの係数(リニア RGB)。エミッシブテクスチャと khr_.emissiveStrength_ に掛ける。
		Float emissiveFactor_[3] = { 0,0,0 };

		/// [EN] glTF alpha mode: 0 opaque, 1 mask (cut at alphaCutoff_), 2 blend.
		/// [JP] glTF のアルファモード。0 が不透明、1 がマスク(alphaCutoff_ で切り抜く)、2 がブレンド。
		Int alphaMode_ = 0;

		/// [EN] Alpha below which a pixel is discarded in mask mode.
		/// [JP] マスクモードで、これ未満のアルファのピクセルを捨てる。
		Float alphaCutoff_ = 0.5f;

		/// [EN] Nonzero to draw both faces without back-face culling.
		/// [JP] 0 以外なら裏面カリングせず両面を描く。
		Int doubleSided_ = 0;

		/// [EN] Lighting response the deferred lighting pass uses for this material.
		/// [JP] デフォードライティングパスがこのマテリアルに使うライティング応答。
		ShadingModel shadingModel_ = ShadingModel::Pbr;

		/// [EN] KHR_materials_* extensions of this material.
		/// [JP] このマテリアルの KHR_materials_* 拡張。
		KHR khr_;

		/// [EN] Length of the fur, the distance between the base surface and the outermost shell, in model units.
		/// [JP] 毛の長さ。元の表面から最も外側のシェルまでの距離(モデル空間の単位)。
		Float furLength_ = 0.03f;

		/// [EN] Density of the fur strands.
		/// [JP] 毛の密度。
		Float furDensity_ = 1.0f;

		/// [EN] Number of shells the fur is drawn with; more shells give smoother strands at a higher cost.
		/// [JP] 毛を描くシェルの数。多いほど毛がなめらかになるが、負荷も上がる。
		Int furShellCount_ = 16;

		/// [EN] Base color image in bitmaps_; SC_INVALID when absent.
		/// [JP] ベースカラーの画像(bitmaps_)。無ければ SC_INVALID。
		Uint32 baseColorTextureIndex_ = SC_INVALID;

		/// [EN] Tangent-space normal map in bitmaps_; SC_INVALID when absent.
		/// [JP] 接線空間の法線マップ(bitmaps_)。無ければ SC_INVALID。
		Uint32 normalTextureIndex_ = SC_INVALID;

		/// [EN] Metallic (blue) and roughness (green) image in bitmaps_; SC_INVALID when absent.
		/// [JP] メタリック(B)とラフネス(G)の画像(bitmaps_)。無ければ SC_INVALID。
		Uint32 metallicRoughnessTextureIndex_ = SC_INVALID;

		/// [EN] Ambient occlusion image (red) in bitmaps_; SC_INVALID when absent.
		/// [JP] アンビエントオクルージョンの画像(R、bitmaps_)。無ければ SC_INVALID。
		Uint32 occlusionTextureIndex_ = SC_INVALID;

		/// [EN] Emissive color image in bitmaps_; SC_INVALID when absent.
		/// [JP] エミッシブ色の画像(bitmaps_)。無ければ SC_INVALID。
		Uint32 emissiveTextureIndex_ = SC_INVALID;

		/// [EN] Texture asset that replaces baseColorTextureIndex_ when nonzero.
		/// [JP] 0 以外なら baseColorTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 baseColorTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces normalTextureIndex_ when nonzero.
		/// [JP] 0 以外なら normalTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 normalTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces metallicRoughnessTextureIndex_ when nonzero.
		/// [JP] 0 以外なら metallicRoughnessTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 metallicRoughnessTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces occlusionTextureIndex_ when nonzero.
		/// [JP] 0 以外なら occlusionTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 occlusionTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces emissiveTextureIndex_ when nonzero.
		/// [JP] 0 以外なら emissiveTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 emissiveTextureAssetID_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("name", name_);
			archive.TryField("base_color", baseColor_);
			archive.TryField("metallic", metallic_);
			archive.TryField("roughness", roughness_);
			archive.TryField("emissive_factor", emissiveFactor_);
			archive.TryField("alpha_mode", alphaMode_);
			archive.TryField("alpha_cutoff", alphaCutoff_);
			archive.TryField("double_sided", doubleSided_);
			archive.TryField("shading_model", shadingModel_);
			archive.TryField("khr", khr_);
			archive.TryField("fur_length", furLength_);
			archive.TryField("fur_density", furDensity_);
			archive.TryField("fur_shell_count", furShellCount_);
			archive.TryField("base_color_texture_index", baseColorTextureIndex_);
			archive.TryField("normal_texture_index", normalTextureIndex_);
			archive.TryField("metallic_roughness_texture_index", metallicRoughnessTextureIndex_);
			archive.TryField("occlusion_texture_index", occlusionTextureIndex_);
			archive.TryField("emissive_texture_index", emissiveTextureIndex_);
			archive.TryField("base_color_texture_asset_id", baseColorTextureAssetID_);
			archive.TryField("normal_texture_asset_id", normalTextureAssetID_);
			archive.TryField("metallic_roughness_texture_asset_id", metallicRoughnessTextureAssetID_);
			archive.TryField("occlusion_texture_asset_id", occlusionTextureAssetID_);
			archive.TryField("emissive_texture_asset_id", emissiveTextureAssetID_);
		}
	};

	/**
	* [EN]
	* One node in the flattened glTF node hierarchy: local S/R/T, an
	* optional mesh_/skin_/light_ reference, and children_ indices into
	* nodes_. globalTransform_ is cumulated top-down by
	* CumulateTransforms()/ModelLoader::CumulateTransforms from every
	* ancestor's local transform.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 平坦化された glTF ノード階層の1ノード: ローカル S/R/T、任意の
	* mesh_/skin_/light_ 参照、nodes_ を指す children_ インデックス。
	* globalTransform_ は CumulateTransforms()/
	* ModelLoader::CumulateTransforms が全祖先のローカルトランスフォーム
	* から上から下へ累積計算する。
	*/
	struct Node
	{
		/// [EN] Node name from the source file; not guaranteed unique.
		/// [JP] 元ファイルでのノード名。一意とは限らない。
		std::string name_;

		/// [EN] Source mesh this node places, matched against SubMesh::meshIndex_; -1 if none.
		/// [JP] このノードが配置する元のメッシュ。SubMesh::meshIndex_ と照合する。無ければ -1。
		Int mesh_ = -1;

		/// [EN] Index into Crister::skins_ of the skin that deforms this node's mesh; -1 if none.
		/// [JP] このノードのメッシュを変形するスキンの、Crister::skins_ でのインデックス。無ければ -1。
		Int skin_ = -1;

		/// [EN] KHR_lights_punctual: index into Crister::lights_, -1 if none.
		/// [JP] KHR_lights_punctual: Crister::lights_ へのインデックス。無ければ -1。
		Int light_ = -1;

		/// [EN] Indices into nodes_ of this node's children.
		/// [JP] このノードの子の、nodes_ でのインデックス。
		DynamicArray<Int> children_;

		/// [EN] Index into nodes_ of this node's parent; -1 for a root node.
		/// [JP] このノードの親の、nodes_ でのインデックス。ルートノードなら -1。
		Int parentIndex_ = -1;

		/// [EN] Local rotation relative to the parent.
		/// [JP] 親に対するローカル回転。
		Quaternion rotation_ = { 0,0,0,1 };

		/// [EN] Local scale relative to the parent.
		/// [JP] 親に対するローカルスケール。
		Vector3 scale_ = { 1,1,1 };

		/// [EN] Local translation relative to the parent.
		/// [JP] 親に対するローカル平行移動。
		Vector3 translation_ = { 0,0,0 };

		/// [EN] Model-space transform, the local S/R/T multiplied by every ancestor's.
		/// [JP] モデル空間での変換。ローカルの S/R/T に全ての祖先のものを掛けたもの。
		Matrix globalTransform_ = Matrix::Identity;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("mesh", mesh_);
			archive.Field("skin", skin_);
			archive.Field("light", light_);
			archive.Field("children", children_);
			archive.Field("rotation", rotation_);
			archive.Field("scale", scale_);
			archive.Field("translation", translation_);
			archive.Field("global_transform", globalTransform_);
		}
	};

	/**
	* [EN]
	* glTF skin: the joint list (indices into nodes_) and their matching
	* inverse-bind matrices, indexed 1:1. Referenced by Node::skin_ and
	* SubMesh::skinIndex_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF のスキン: ジョイント一覧（nodes_ へのインデックス）と、それに
	* 1対1対応する逆バインド行列。Node::skin_ と SubMesh::skinIndex_ から
	* 参照される。
	*/
	struct Skin
	{
		/// [EN] Inverse bind matrix of each joint, taking a model-space bind-pose vertex into that joint's space; index-aligned with joints_.
		/// [JP] 各ジョイントの逆バインド行列。バインドポーズのモデル空間の頂点を、そのジョイントの空間へ移す。joints_ とインデックスが揃っている。
		DynamicArray<Matrix> inverseBindMatrices_;

		/// [EN] Indices into nodes_ of the joints, in the order CompressedSkinVertex joint indices refer to them.
		/// [JP] ジョイントの nodes_ でのインデックス。CompressedSkinVertex のジョイントインデックスが指す順に並ぶ。
		DynamicArray<Int> joints_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("inverse_bind_matrices", inverseBindMatrices_);
			archive.Field("joints", joints_);
		}
	};

	/**
	* [EN]
	* KHR_lights_punctual point/spot light, resolved to world-space
	* position/direction (via the referencing Node's globalTransform_)
	* at load time. Directional lights are not represented here; see
	* ModelLoader::FetchLights.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* KHR_lights_punctual のポイント/スポットライト。ロード時に参照ノードの
	* globalTransform_ からワールド空間の位置/向きに解決済み。
	* ディレクショナルライトはここに含めない（ModelLoader::FetchLights 参照）。
	*/
	struct PunctualLight
	{
		/**
		* [EN]
		* Which KHR_lights_punctual light kind this PunctualLight represents.
		* Directional is not a value here since directional lights are not
		* represented by PunctualLight at all (see the struct comment above).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この PunctualLight が表す KHR_lights_punctual のライト種別。
		* ディレクショナルは PunctualLight そのもので表現しない
		* （上の構造体コメント参照）ため、値として存在しない。
		*/
		enum class Type : Uint32
		{
			/// [EN] Omnidirectional point light.
			/// [JP] 全方向へ発光するポイントライト。
			Point = 0,

			/// [EN] Cone-shaped spot light (uses direction_/innerConeAngle_/outerConeAngle_).
			/// [JP] 円錐状に発光するスポットライト（direction_/innerConeAngle_/outerConeAngle_ を使う）。
			Spot = 1,
		};

		/// [EN] Kind of light.
		/// [JP] ライトの種類。
		Type type_ = Type::Point;

		/// [EN] Position of the light, resolved from its node.
		/// [JP] ライトの位置。ノードから解決したもの。
		Vector3 position_ = { 0,0,0 };

		/// [EN] Direction the spot light points in; used by Spot only.
		/// [JP] スポットライトが向く方向。Spot だけが使う。
		Vector3 direction_ = { 0,0,-1 };

		/// [EN] Linear RGB color of the light.
		/// [JP] ライトの色(リニア RGB)。
		Float colorRGB_[3] = { 1,1,1 };

		/// [EN] Luminous intensity in candela.
		/// [JP] 光度(カンデラ)。
		Float intensity_ = 1.0f;

		/// [EN] Distance at which the light's influence ends. glTF uses 0 for unbounded, which FetchLights resolves to a finite fallback.
		/// [JP] ライトの影響が届かなくなる距離。glTF では 0 が無限遠を表し、FetchLights がそれを有限の代わりの値に解決する。
		Float range_ = 0.0f;

		/// [EN] Angle from the spot direction where the falloff begins, in radians; used by Spot only.
		/// [JP] スポットの方向から測った、減衰が始まる角度(ラジアン)。Spot だけが使う。
		Float innerConeAngle_ = 0.0f;

		/// [EN] Angle from the spot direction where the light reaches zero, in radians; 45 degrees by default as in the glTF specification. Used by Spot only.
		/// [JP] スポットの方向から測った、光が 0 になる角度(ラジアン)。既定は glTF 仕様と同じ 45 度。Spot だけが使う。
		Float outerConeAngle_ = 0.785398163f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("type", type_);
			archive.Field("position", position_);
			archive.Field("direction", direction_);
			archive.Field("color_rgb", colorRGB_);
			archive.Field("intensity", intensity_);
			archive.Field("range", range_);
			archive.Field("inner_cone_angle", innerConeAngle_);
			archive.Field("outer_cone_angle", outerConeAngle_);
		}
	};

	/**
	* [EN]
	* width_/height_/mipCount_ describe cacheData_ as baked by
	* Crister::BakeBitmap(): BC7-compressed blocks for mip 0 through
	* mipCount_-1, concatenated in mip order (standard BC block
	* layout — 16 bytes per 4x4 texel block, no explicit per-mip
	* offsets stored; Upload() recomputes them from width_/height_).
	* component_/bits_ describe the original glTF source image only.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_/height_/mipCount_ は Crister::BakeBitmap() が焼いた
	* cacheData_ の内容を表す: mip 0 から mipCount_-1 までの BC7 圧縮
	* ブロックをミップ順に連結したもの（標準的な BC ブロックレイアウト
	* — 4x4 テクセルブロックあたり 16 バイト、ミップごとのオフセットは
	* 持たず Upload() が width_/height_ から再計算する）。
	* component_/bits_ は元の glTF ソース画像の情報のみ。
	*/
	struct Bitmap
	{
		/// [EN] Image name from the source file.
		/// [JP] 元ファイルでの画像名。
		std::string name_;

		/// [EN] Width of mip 0 in cacheData_, in texels.
		/// [JP] cacheData_ のミップ 0 の幅(テクセル)。
		Int width_ = -1;

		/// [EN] Height of mip 0 in cacheData_, in texels.
		/// [JP] cacheData_ のミップ 0 の高さ(テクセル)。
		Int height_ = -1;

		/// [EN] Channel count of the source image.
		/// [JP] 元画像のチャンネル数。
		Int component_ = -1;

		/// [EN] Bits per channel of the source image.
		/// [JP] 元画像のチャンネルあたりのビット数。
		Int bits_ = -1;

		/// [EN] Number of mip levels in cacheData_.
		/// [JP] cacheData_ に入っているミップの段数。
		Int mipCount_ = 1;

		/// [EN] MIME type of the source image, such as image/png.
		/// [JP] 元画像の MIME タイプ(image/png など)。
		std::string mimeType_;

		/// [EN] BC7 blocks of every mip, mip 0 first.
		/// [JP] 全ミップの BC7 ブロック。ミップ 0 から順に並ぶ。
		DynamicArray<Uchar> cacheData_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("width", width_);
			archive.Field("height", height_);
			archive.Field("component", component_);
			archive.Field("bits", bits_);
			archive.Field("mip_count", mipCount_);
			archive.Field("mime_type", mimeType_);
			archive.Field("cache_data", cacheData_);
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
	* GPU-resident, geometry/texture-streaming model asset: quantised
	* mesh data (compressedVertices_ etc.), meshlet/cluster LOD hierarchy,
	* materials/nodes/skins/lights, and the .crister-cache-backed bake
	* pipeline (BakeMesh/BakeBitmap) that produces them. Built once by
	* ModelLoader from a source glTF (or reloaded straight from a
	* .crister cache), then Upload()ed to create its D3D12 resources and
	* bindless indices. Owns its own Nanite-style streaming residency
	* (MakeClusterResident/EvictCluster, MakeTextureMipResident/
	* EvictTextureMip) against shared VRAM budgets tracked across every
	* live Crister.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GPU 常駐・ジオメトリ/テクスチャストリーミング対応のモデルアセット:
	* 量子化済みメッシュデータ (compressedVertices_ 等)、meshlet/cluster
	* の LOD 階層、マテリアル/ノード/スキン/ライト、そしてそれらを生成する
	* .crister キャッシュ向けのベイクパイプライン (BakeMesh/BakeBitmap)。
	* ModelLoader がソース glTF から一度構築する（あるいは .crister
	* キャッシュから直接リロードする）。その後 Upload() で D3D12
	* リソースと bindless インデックスを作成する。独自の Nanite 型
	* ストリーミング常駐管理 (MakeClusterResident/EvictCluster、
	* MakeTextureMipResident/EvictTextureMip) を持ち、生存中の全 Crister
	* で共有する VRAM 予算に対して管理する。
	*/
	class SEEDCORE_API Crister
	{
	private:
		friend class ModelLoader;
		friend class ModelExporter;

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

		/// [EN] Scenes of the source file.
		/// [JP] 元ファイルのシーン。
		DynamicArray<Stage> stages_;

		/// [EN] Drawable pieces of the model, one per source primitive.
		/// [JP] モデルの描画可能なパーツ。元のプリミティブ1つにつき1つ。
		DynamicArray<SubMesh> subMeshes_;

		/// [EN] Materials, referred to by SubMesh::surfaceIndex_.
		/// [JP] マテリアル。SubMesh::surfaceIndex_ から参照される。
		DynamicArray<Surface> surfaces_;

		/// [EN] Flattened node hierarchy of the source file.
		/// [JP] 元ファイルのノード階層を平坦化したもの。
		DynamicArray<Node> nodes_;

		/// [EN] Skins, referred to by Node::skin_ and SubMesh::skinIndex_.
		/// [JP] スキン。Node::skin_ と SubMesh::skinIndex_ から参照される。
		DynamicArray<Skin> skins_;

		/// [EN] Embedded images, referred to by the texture indices in Surface.
		/// [JP] 内蔵画像。Surface のテクスチャインデックスから参照される。
		DynamicArray<Bitmap> bitmaps_;

		/// [EN] Point and spot lights of the source file.
		/// [JP] 元ファイルのポイントライトとスポットライト。
		DynamicArray<PunctualLight> lights_;

		/// [EN] Index into stages_ of the scene drawn when none is chosen.
		/// [JP] シーンが選ばれていないときに描画する、stages_ でのインデックス。
		Int defaultStage_ = 0;

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
		* Releases every resident streaming page (pinned included — the model
		* itself is going away) and returns their descriptor indices to
		* bindlessHeap_. Every GPU resource is handed to the deferred-reclaim
		* ring rather than dying with this object, since frames still in
		* flight may be drawing from these exact buffers and textures.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 常駐中のストリーミングページをすべて解放し（モデル自体が消えるため
		* ピン留め込み）、ディスクリプタインデックスを bindlessHeap_ へ返す。
		* GPU リソースはこのオブジェクトと一緒に死なせず、遅延回収リングへ
		* 渡す — インフライトのフレームがまさにこれらのバッファやテクスチャで
		* 描画している可能性があるため。
		*/
		~Crister();

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
			archive.Field("compressed_vertices", compressedVertices_);
			archive.Field("compressed_skin_vertices", compressedSkinVertices_);
			archive.Field("vertex_morph_source", vertexMorphSource_);
			archive.Field("position_min", positionMin_);
			archive.Field("position_extent", positionExtent_);
			archive.Field("texcoord_min", texcoordMin_);
			archive.Field("texcoord_extent", texcoordExtent_);
			archive.Field("vertex_indices", vertexIndices_);
			archive.Field("primitive_indices", primitiveIndices_);
			archive.Field("meshlets", meshlets_);
			archive.Field("meshlet_bounds", meshletBounds_);
			archive.Field("clusters", clusters_);
			archive.Field("stages", stages_);
			archive.Field("sub_meshes", subMeshes_);
			archive.Field("surfaces", surfaces_);
			archive.Field("nodes", nodes_);
			archive.Field("skins", skins_);
			archive.Field("bitmaps", bitmaps_);
			archive.Field("lights", lights_);
			archive.Field("default_stage", defaultStage_);
		}

		/**
		* [EN]
		* Computes the quantisation AABB and bakes vertices_ (and, if
		* skinned, skin attributes) into compressedVertices_ /
		* compressedSkinVertices_, then frees vertices_. Must run after
		* BuildMeshlets (LOD vertex duplication must already be final)
		* and before serialising to the .crister cache.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 量子化 AABB を計算し、vertices_（スキンがあればスキン属性も）を
		* compressedVertices_ / compressedSkinVertices_ へ焼き込んで
		* vertices_ を解放する。BuildMeshlets の後（LOD 頂点複製が確定済み）、
		* .crister キャッシュへのシリアライズ前に実行すること。
		*/
		void BakeMesh();

		/**
		* [EN]
		* Downsamples oversized textures, dilates transparent-texel color
		* (before compression, since BC7 blocks cannot be edited per texel
		* afterwards), then BC7-compresses a full mip chain into each
		* Bitmap's cacheData_. Must run before serialising to the .crister
		* cache. The BC7 encoding runs on the GPU (BC7CompressCS.hlsl, mode
		* 6 only) as a one-shot compute dispatch on cmdQueue, with the root
		* signature and PSO owned by bc7Shader. The shader binds its input
		* and output as root SRV/UAV, so unlike Upload() this needs no
		* BindlessHeap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 大きすぎるテクスチャをダウンサンプルし、透明テクセルの色を
		* dilation する（圧縮後は BC7 ブロックをテクセル単位で編集できない
		* ため、圧縮前に行う）。その後フルミップチェーンを BC7 圧縮して各
		* Bitmap の cacheData_ へ焼き込む。.crister キャッシュへの
		* シリアライズ前に実行すること。BC7 エンコードは GPU で行い
		* (BC7CompressCS.hlsl、mode 6 のみ)、cmdQueue 上の一発実行の
		* コンピュートディスパッチとして、bc7Shader が持つルートシグネチャと
		* PSO で動かす。シェーダーは入出力をルート SRV/UAV でバインドするので、
		* Upload() と違い BindlessHeap は要らない。
		*/
		void BakeBitmap(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader);

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
		* Quantises a single full-precision Vertex into the 16-byte GPU
		* format, given the position/texcoord dequantisation AABBs to
		* quantise against. Extracted out of BakeMesh()'s per-vertex loop
		* body so SoftbodyMesh can re-quantise a Softbody's deformed
		* vertices every frame against a freshly computed AABB (the mesh
		* moves, so a fixed bake-time AABB would clip) using the exact same
		* math BakeMesh() bakes into the cache with — must stay in sync with
		* Model.hlsli's DecodeModelVertex, same as BakeMesh().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フル精度の Vertex 1 つを、指定された位置/UV の逆量子化 AABB に
		* 対して 16 バイトの GPU フォーマットへ量子化する。BakeMesh() の
		* 頂点ループ本体から切り出したもの — SoftbodyMesh が Softbody の
		* 変形後頂点を毎フレーム、その都度計算した AABB（メッシュが動くため
		* 焼き込み時固定の AABB ではクリップしてしまう）に対して、
		* BakeMesh() がキャッシュへ焼き込むのと全く同じ計算式で再量子化
		* できるようにする — BakeMesh() と同様、Model.hlsli の
		* DecodeModelVertex と同期を保つこと。
		*/
		NODISCARD static CompressedVertex EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent);

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
		* Packs a unit direction into the 16+16-bit octahedral encoding
		* (Shader/Normal.hlsli::OctNormalEncode's CPU-side counterpart).
		* Static for the same reason as EncodeVertex: SoftbodyMesh calls this
		* indirectly via EncodeVertex with its own bounds, not this Crister's.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位方向ベクトルを 16+16bit のオクタヘドラル符号化へ詰める
		* （Shader/Normal.hlsli::OctNormalEncode の CPU 側対応）。EncodeVertex
		* と同じ理由で static — SoftbodyMesh が EncodeVertex 経由で、この
		* Crister とは別の自身の境界を使って間接的に呼ぶため。
		*/
		NODISCARD static Uint32 EncodeOctahedralNormal(Vector3 direction);

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
		* Re-converts an already-baked Crister (no source glTF required) from
		* one axis convention to another, in place: decodes every quantised
		* vertex/skin back to full precision, applies deltaBasis to
		* positions/normals/tangents/node transforms/skin inverse-bind
		* matrices/light positions-directions/meshlet bounds, optionally
		* reverses triangle winding, recomputes the quantisation AABBs from
		* the transformed data, re-quantises via BakeMesh(), and
		* re-serialises the result to cristerPath. Does NOT touch GPU
		* resources or bindless indices — the caller must force a reload
		* (Unload then Load) afterward. Returns false if this Crister has no
		* compressed vertex data to convert (e.g. textures-only/degenerate asset).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 既に焼き込み済みの Crister（ソース glTF 不要）を、ある軸コンベンション
		* から別のものへその場で再変換する: 量子化済みの頂点/スキンをフル精度へ
		* デコードし、位置/法線/タンジェント/ノードトランスフォーム/スキン
		* 逆バインド行列/ライト位置・向き/メシュレット境界へ deltaBasis を
		* 適用、必要なら三角形の巻き順を反転、変換後データから量子化 AABB を
		* 再計算し、BakeMesh() で再量子化して cristerPath へ再シリアライズ
		* する。GPU リソースや bindless インデックスは触らない —
		* 呼び出し側が後で強制リロード（Unload → Load）すること。この
		* Crister に変換対象の量子化済み頂点データが無い場合（テクスチャのみ
		* 等の縮退アセット）は false を返す。
		*/
		Bool ApplyAxisConversion(const Matrix& deltaBasis, Bool flipWinding, const std::filesystem::path& cristerPath);

		/**
		* [EN]
		* Applies a global position/rotation(euler degrees)/scale/pivot
		* transform to this Crister's node hierarchy and light
		* positions-directions, then re-serialises to cristerPath. Vertices,
		* skin inverse-bind matrices and meshlet bounds stay in mesh-local
		* space: static SubMeshes are placed by their node's global transform
		* at draw time (SubMeshPlacement) and skinned ones through their
		* joints, so both pick the transform up from the nodes.
		* scale/pivot/rotation compose about pivot first, position is a
		* separate world-space offset applied after. Only root-level nodes
		* (stages_[defaultStage_].nodes_) have their local transform
		* updated - CumulateTransforms() then propagates to every
		* descendant, since post-multiplying the whole transform onto just
		* the root telescopes correctly through the local-transform chain
		* (node.globalTransform_ = local * parentGlobal). Returns false if
		* this Crister has no compressed vertex data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* グローバルな位置/回転(オイラー角、度)/スケール/ピボット変換を
		* この Crister のノード階層とライト位置・向きへ適用し、その後
		* cristerPath へ再シリアライズする。頂点・スキン逆バインド行列・
		* メシュレット境界はメッシュローカル空間のまま: 静的 SubMesh は描画時に
		* ノードのグローバル変換で配置され(SubMeshPlacement)、スキン付きは
		* ジョイント経由なので、どちらもノードから変換を受け取る。
		* スケール/ピボット/回転はまずピボットを中心に合成し、position は
		* その後に適用する独立したワールド空間オフセット。ローカル
		* トランスフォームを更新するのはルートノード
		* (stages_[defaultStage_].nodes_)のみ — CumulateTransforms() が
		* 全子孫へ伝播する。ルートだけに変換全体を後乗せすれば、ローカル
		* トランスフォームの連鎖(node.globalTransform_ = local *
		* parentGlobal)を通じて正しく telescope するため。この Crister に
		* 変換対象の量子化済み頂点データが無い場合は false を返す。
		*/
		Bool ApplyTransformConversion(Vector3 position, Vector3 rotation, Vector3 scale, Vector3 pivot, const std::filesystem::path& cristerPath);

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
		NODISCARD const DynamicArray<Stage>& Stages()const;

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
		NODISCARD const DynamicArray<Node>& Nodes()const;

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
		NODISCARD const DynamicArray<PunctualLight>& Lights()const;

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
		NODISCARD const DynamicArray<SubMesh>& SubMeshes()const;

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
		NODISCARD const DynamicArray<Surface>& Surfaces()const;

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
		NODISCARD const DynamicArray<Skin>& Skins()const;

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
		NODISCARD const DynamicArray<Cluster>& Clusters()const;

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
		NODISCARD const DynamicArray<Matrix>& SubMeshPlacement(Size subMeshIndex)const;

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
		* recomputes every Node::globalTransform_ from local S/R/T.
		* Duplicates ModelLoader::CumulateTransforms' logic so
		* ApplyAxisConversion can recompute global transforms without a
		* ModelLoader/glTF re-parse.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* stages_[defaultStage_]のノードツリーを深さ優先で走査し、ローカル
		* S/R/T から全 Node::globalTransform_ を再計算する。
		* ModelLoader::CumulateTransforms のロジックを複製したもの。
		* ApplyAxisConversion が ModelLoader/glTF 再解析無しにグローバル
		* トランスフォームを再計算できるようにする。
		*/
		void CumulateTransforms();

		/**
		* [EN]
		* Allocates a bindless heap index and creates a StructuredBuffer SRV
		* for resource at that index.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* bindless ヒープインデックスを1つ確保し、resource に対する
		* StructuredBuffer SRV をそのインデックスへ作成する。
		*/
		static Uint CreateStructuredShaderResourceView(ID3D12Device* device, BindlessHeap* heap, ID3D12Resource* resource, Uint elementCount, Uint stride);

		/**
		* [EN]
		* Creates a DEFAULT-heap buffer filled with data, the same way
		* DirectX::CreateStaticBuffer does but with no size cap.
		* CreateStaticBuffer rejects anything above ~128MB, because
		* DirectXTK12 treats D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM
		* as a flat byte limit; D3D12 buffers themselves are bounded only by
		* GPU memory, and a high-poly mesh's flat 32-bit triangle index
		* buffer easily goes past 128MB.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data を詰めた DEFAULT ヒープのバッファを、DirectX::CreateStaticBuffer
		* と同じ手順で、ただしサイズの上限なしで作る。CreateStaticBuffer は
		* DirectXTK12 が D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM
		* を単純なバイト上限として扱うため、約128MB を超えるものを拒否する。
		* D3D12 のバッファ自体の上限は GPU メモリだけで、ハイポリメッシュの
		* フラットな 32bit 三角形インデックスバッファは 128MB を簡単に超える。
		*/
		static HRESULT CreateStaticBufferUnbounded(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const void* data, Uint64 count, Uint64 stride, D3D12_RESOURCE_STATES afterState, ID3D12Resource** outResource);

		/**
		* [EN]
		* Texture counterpart to CreateStaticBufferUnbounded's byte-layout
		* role: resolves mip dimensions/row-pitch/slice-pitch/byte-offset
		* within Bitmap::cacheData_ for a given mip index, from the standard
		* BC block layout (16 bytes per 4x4 texel block, mips concatenated
		* in order with no stored per-mip offsets).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CreateStaticBufferUnbounded のバイトレイアウト計算に相当する
		* テクスチャ版: Bitmap::cacheData_ 内での指定ミップの寸法/
		* row-pitch/slice-pitch/バイトオフセットを、標準的な BC
		* ブロックレイアウト（4x4 テクセルブロックあたり 16 バイト、
		* ミップは順に連結・オフセット未保存）から解決する。
		*/
		static void ComputeTextureMipLayout(const Bitmap& bitmap, Uint32 mipIndex, Uint32& outWidth, Uint32& outHeight, Uint64& outRowPitch, Uint64& outSlicePitch, Uint64& outByteOffset);

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

		/**
		* [EN]
		* Change-of-basis for a position: transforms vector in place by basis.
		* Duplicates ModelLoader's ConvertPositionByBasis (private to
		* ModelLoader) so ApplyAxisConversion can re-convert without a
		* ModelLoader/glTF re-parse.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 位置の基底変換。vector を basis でその場変換する。ModelLoader の
		* ConvertPositionByBasis（ModelLoader 限定 private）を複製し、
		* ApplyAxisConversion が ModelLoader/glTF 再解析無しに再変換できる
		* ようにする。
		*/
		void ConvertPositionByBasis(Vector3& vector, const Matrix& basis)const;

		/**
		* [EN]
		* Change-of-basis for a rotation: transforms quaternion in place by basis.
		* Duplicates ModelLoader's ConvertRotationByBasis (private to ModelLoader).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回転の基底変換。quaternion を basis でその場変換する。ModelLoader の
		* ConvertRotationByBasis（ModelLoader 限定 private）を複製する。
		*/
		void ConvertRotationByBasis(Quaternion& quaternion, const Matrix& basis)const;

		/**
		* [EN]
		* Change-of-basis for a matrix: transforms matrix in place by basis.
		* Duplicates ModelLoader's ConvertMatrixByBasis (private to ModelLoader).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 行列の基底変換。matrix を basis でその場変換する。ModelLoader の
		* ConvertMatrixByBasis（ModelLoader 限定 private）を複製する。
		*/
		void ConvertMatrixByBasis(Matrix& matrix, const Matrix& basis)const;

		/**
		* [EN]
		* Quantises a [0,1] float to a 16-bit UNORM, clamping out-of-range
		* input first. Shared by EncodeVertex (position/texcoord) and
		* EncodeOctahedralNormal.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* [0,1] の float を 16bit UNORM へ量子化する。範囲外の入力は先に
		* クランプする。EncodeVertex（position/texcoord）と
		* EncodeOctahedralNormal が共有する。
		*/
		NODISCARD static Uint32 QuantizeUnorm16(Float value01);

		/**
		* [EN]
		* Quantises a [0,1] float to an 8-bit UNORM, clamping out-of-range
		* input first. Used by BakeMesh for skin weights.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* [0,1] の float を 8bit UNORM へ量子化する。範囲外の入力は先に
		* クランプする。BakeMesh がスキンウェイトに使う。
		*/
		NODISCARD static Uint32 QuantizeUnorm8(Float value01);

		/**
		* [EN]
		* One streamable GPU page: a Cluster's rebased slice of geometry.
		* Offsets stored in the page's meshlets are page-local; LOD>=1
		* pages own their vertex slice, LOD 0 pages reference the shared
		* vertex pool page instead (vertexBufferIndex_ = pool SRV).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ストリーミング可能な GPU ページ 1 つ = Cluster のリベース済み
		* ジオメトリスライス。ページ内 meshlet のオフセットはページ
		* ローカル。LOD>=1 は専用頂点スライスを所有し、LOD 0 は共有頂点
		* プールページを参照する（vertexBufferIndex_ = プール SRV）。
		*/
		struct StreamingGeometry
		{
			/// [EN] Start of the page's slice of vertexIndices_, derived from its meshlets when Upload() runs.
			/// [JP] ページが持つ vertexIndices_ のスライスの先頭。Upload() で meshlet から導出する。
			Uint32 vertexIndexBegin_ = 0;

			/// [EN] End (exclusive) of the page's slice of vertexIndices_.
			/// [JP] ページが持つ vertexIndices_ のスライスの終わり(含まない)。
			Uint32 vertexIndexEnd_ = 0;

			/// [EN] Start of the page's slice of primitiveIndices_, in bytes.
			/// [JP] ページが持つ primitiveIndices_ のスライスの先頭(バイト)。
			Uint32 primitiveBegin_ = 0;

			/// [EN] End (exclusive) of the page's slice of primitiveIndices_, in bytes.
			/// [JP] ページが持つ primitiveIndices_ のスライスの終わり(バイト、含まない)。
			Uint32 primitiveEnd_ = 0;

			/// [EN] Start of the vertex range the page owns in compressedVertices_; used only when ownsVertices_.
			/// [JP] ページが compressedVertices_ に持つ頂点範囲の先頭。ownsVertices_ のときだけ使う。
			Uint32 vertexBegin_ = 0;

			/// [EN] End (exclusive) of that vertex range.
			/// [JP] その頂点範囲の終わり(含まない)。
			Uint32 vertexEnd_ = 0;

			/// [EN] Whether the page carries its own vertex slice (LOD >= 1) instead of referencing the shared LOD 0 pool.
			/// [JP] 共有 LOD 0 プールを参照するのではなく、自前の頂点スライスを持つか(LOD >= 1)。
			Bool ownsVertices_ = false;

			/// [EN] Whether the page must never be evicted (the coarsest cluster of a SubMesh, or LOD 0 of a skinned one).
			/// [JP] 追い出してはいけないページか(SubMesh の最粗クラスタか、スキン付き SubMesh の LOD 0)。
			Bool pinned_ = false;

			/// [EN] The page's meshlets, with offsets rebased to page-local numbering.
			/// [JP] ページの meshlet。オフセットはページローカルな番号へリベース済み。
			Microsoft::WRL::ComPtr<ID3D12Resource> meshletResource_;

			/// [EN] Culling bounds of the page's meshlets.
			/// [JP] ページの meshlet のカリング用の境界。
			Microsoft::WRL::ComPtr<ID3D12Resource> meshletBoundResource_;

			/// [EN] The page's slice of vertexIndices_.
			/// [JP] ページが持つ vertexIndices_ のスライス。
			Microsoft::WRL::ComPtr<ID3D12Resource> vertexIndicesResource_;

			/// [EN] The page's slice of primitiveIndices_.
			/// [JP] ページが持つ primitiveIndices_ のスライス。
			Microsoft::WRL::ComPtr<ID3D12Resource> primitiveIndicesResource_;

			/// [EN] The page's own vertices; empty when it references the shared pool.
			/// [JP] ページ自前の頂点。共有プールを参照するページでは空。
			Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;

			/// [EN] Bindless SRV of meshletResource_.
			/// [JP] meshletResource_ の bindless SRV。
			Uint meshletBufferIndex_ = SC_INVALID;

			/// [EN] Bindless SRV of meshletBoundResource_.
			/// [JP] meshletBoundResource_ の bindless SRV。
			Uint meshletBoundBufferIndex_ = SC_INVALID;

			/// [EN] Bindless SRV of vertexIndicesResource_.
			/// [JP] vertexIndicesResource_ の bindless SRV。
			Uint vertexIndicesBufferIndex_ = SC_INVALID;

			/// [EN] Bindless SRV of primitiveIndicesResource_.
			/// [JP] primitiveIndicesResource_ の bindless SRV。
			Uint primitiveIndicesBufferIndex_ = SC_INVALID;

			/// [EN] Bindless SRV of the vertices the page reads: its own vertexResource_, or the shared pool's.
			/// [JP] ページが読む頂点の bindless SRV。自前の vertexResource_ か、共有プールのもの。
			Uint vertexBufferIndex_ = SC_INVALID;

			/// [EN] Whether the page is currently in VRAM.
			/// [JP] ページが今 VRAM にあるか。
			Bool resident_ = false;

			/// [EN] VRAM the page takes while resident, counted against the geometry budget.
			/// [JP] 常駐中にページが使う VRAM。ジオメトリの予算に数える。
			Uint64 sizeBytes_ = 0;

			/// [EN] Last frame the page was drawn from; it is evicted only after evictAgeFrames_ frames without use.
			/// [JP] ページが最後に描画に使われたフレーム。evictAgeFrames_ フレーム使われなかった後でだけ追い出す。
			Uint64 lastUsedFrame_ = 0;
		};

		/**
		* [EN]
		* One streamable GPU mip of a Model-embedded texture: a single-mip
		* committed resource at that mip's own resolution. UV space is the
		* same regardless of which mip currently backs the SRV, so swapping
		* it in/out needs no shader-side change.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Model 内蔵テクスチャのストリーミング可能な GPU ミップ 1 つ。
		* そのミップ自身の解像度を持つ単一ミップの committed resource。
		* どのミップが SRV の裏にあっても UV 空間は変わらないため、
		* 出し入れにシェーダ側の変更は不要。
		*/
		struct TextureMipLevel
		{
			/// [EN] Single-mip BC7 texture at this mip's resolution; null while not resident.
			/// [JP] このミップの解像度を持つ単一ミップの BC7 テクスチャ。常駐していなければ null。
			Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

			/// [EN] Bindless SRV of resource_.
			/// [JP] resource_ の bindless SRV。
			Uint bindlessIndex_ = SC_INVALID;

			/// [EN] VRAM the mip takes, counted against the texture budget.
			/// [JP] このミップが使う VRAM。テクスチャの予算に数える。
			Uint64 sizeBytes_ = 0;
		};

		/**
		* [EN]
		* Streaming state for one Bitmap. At most 2 mips are ever GPU-resident
		* at once: pinnedMip_ (mipCount_-1, the coarsest, uploaded once at
		* Upload() and never freed — guarantees a fallback SRV) and
		* currentMip_ (topResidentMip_, the best mip streamed in so far;
		* unused while topResidentMip_ == mipCount_-1, i.e. nothing finer
		* than pinned has been requested yet). TextureBindlessIndex always
		* returns whichever of the two is finest, so intermediate mips
		* never need to stay resident — only the currently sampled one.
		* Holding two mips per texture keeps bindless-heap and VRAM use
		* bounded; keeping every mip from topResidentMip_ to the coarsest
		* resident would multiply both by up to mipCount_ per texture and
		* exhaust the shared BindlessHeap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Bitmap 1 枚分のストリーミング状態。GPU 常駐は常に最大2ミップまで:
		* pinnedMip_(mipCount_-1、最粗。Upload() で一度だけアップロードし
		* 以後解放しない — フォールバック SRV を保証する)と currentMip_
		* (topResidentMip_、これまでにストリームインした最良ミップ。
		* topResidentMip_ == mipCount_-1 の間、つまりピン留めより細かい
		* ミップを未要求の間は未使用)。TextureBindlessIndex は常にこの2つの
		* うち細かい方を返すため、中間ミップを常駐させ続ける必要はなく、
		* 実際にサンプルされる1枚だけで済む。テクスチャあたり2ミップに
		* 抑えることで、bindless ヒープと VRAM の使用量に上限ができる。
		* topResidentMip_ から最粗までの全ミップを常駐させると、どちらも
		* テクスチャあたり最大 mipCount_ 倍になり、共有 BindlessHeap を
		* 使い切ってしまう。
		*/
		struct StreamingTexture
		{
			/// [EN] The coarsest streamable mip, resident from Upload() until the Crister is destroyed.
			/// [JP] ストリーミングできる中で最も粗いミップ。Upload() から Crister が破棄されるまで常駐する。
			TextureMipLevel pinnedMip_;

			/// [EN] The finest mip streamed in so far; empty while nothing finer than pinnedMip_ has been requested.
			/// [JP] ここまでにストリームインした最も細かいミップ。pinnedMip_ より細かいものが要求されるまでは空。
			TextureMipLevel currentMip_;

			/// [EN] Number of mips that can be streamed, the leading run legal as standalone single-mip BC7 resources.
			/// [JP] ストリーミングできるミップの数。単一ミップの BC7 リソースとして正しい、先頭からの範囲。
			Uint32 mipCount_ = 0;

			/// [EN] Finest mip currently resident; mipCount_ until the pinned mip has been uploaded.
			/// [JP] 今常駐している最も細かいミップ。ピン留めのミップをアップロードするまでは mipCount_。
			Uint32 topResidentMip_ = 0;

			/// [EN] Last frame the texture was sampled; currentMip_ is evicted only after evictAgeFrames_ frames without use.
			/// [JP] テクスチャが最後にサンプルされたフレーム。evictAgeFrames_ フレーム使われなかった後でだけ currentMip_ を追い出す。
			Uint64 lastUsedFrame_ = 0;

			/// [EN] Whether the texture streams at all; false when its dimensions allow no single-mip BC7 resource.
			/// [JP] そもそもストリーミングするテクスチャか。単一ミップの BC7 リソースを作れない寸法なら false。
			Bool valid_ = false;
		};

		/// [EN] RT proxy vertices in the 16-byte GPU format, read by the hit shaders: each SubMesh's LOD 0 cluster, once per node placement. Always resident.
		/// [JP] ヒットシェーダーが読む、16 バイトの GPU フォーマットの RT プロキシ頂点。各 SubMesh の LOD 0 クラスタを、ノード配置ごとに持つ。常に常駐する。
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;

		/// [EN] The same RT proxy vertices decoded to float3 positions, the BLAS input and the base positions of the morph and skin blends.
		/// [JP] 同じ RT プロキシ頂点を float3 の位置へデコードしたもの。BLAS の入力で、モーフとスキンのブレンドのベース位置にもなる。
		Microsoft::WRL::ComPtr<ID3D12Resource> positionResource_;

		/// [EN] Skinning attributes over the vertex pool range, for skinned models; skinned SubMeshes draw only from LOD 0, which indexes the pool.
		/// [JP] スキン付きモデル用の、頂点プール範囲のスキニング属性。スキン付き SubMesh はプールを参照する LOD 0 からしか描画しない。
		Microsoft::WRL::ComPtr<ID3D12Resource> skinVertexResource_;

		/// [EN] Flat 32-bit triangle indices of the RT proxy, shared by the BLAS and the hit shaders.
		/// [JP] RT プロキシのフラットな 32bit 三角形インデックス。BLAS とヒットシェーダーが共有する。
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

		/// [EN] Skinning attributes of each RT proxy vertex, index-aligned with vertexResource_.
		/// [JP] RT プロキシの各頂点のスキニング属性。vertexResource_ とインデックスが揃っている。
		Microsoft::WRL::ComPtr<ID3D12Resource> raytracingSkinVertexResource_;

		/// [EN] Flat float3 morph-target delta pool for the RT proxy, laid out per target within each SubMesh's compact vertex range (see SubMesh::raytracingMorphDeltaOffset_/raytracingVertexOffset_/raytracingVertexCount_). Allocated only when at least one SubMesh has morphs_.
		/// [JP] RT プロキシ用のフラットな float3 モーフターゲットデルタプール。各 SubMesh のコンパクトな頂点範囲の中で、ターゲットごとに並ぶ(SubMesh::raytracingMorphDeltaOffset_/raytracingVertexOffset_/raytracingVertexCount_ 参照)。いずれかの SubMesh が morphs_ を持つときだけ確保する。
		Microsoft::WRL::ComPtr<ID3D12Resource> raytracingMorphDeltaResource_;

		/// [EN] Raster morph delta pool, laid out per target for each SubMesh (SubMesh::morphDeltaOffset_) and baked straight from Morph::positionDeltas_. Read through the bindless heap by the model mesh shaders; allocated only when at least one SubMesh has morphs_.
		/// [JP] ラスタ用のモーフデルタプール。SubMesh ごとにターゲット順で並び(SubMesh::morphDeltaOffset_)、Morph::positionDeltas_ からそのまま焼き込む。モデル用メッシュシェーダーが bindless ヒープから読む。いずれかの SubMesh が morphs_ を持つときだけ確保する。
		Microsoft::WRL::ComPtr<ID3D12Resource> morphDeltaResource_;

		/// [EN] GPU copy of vertexMorphSource_, mapping any streamed LOD's vertex back to the vertex morphDeltaResource_ is indexed by. Allocated only when at least one SubMesh has morphs_.
		/// [JP] vertexMorphSource_ の GPU 側のコピー。ストリーミングされたどの LOD の頂点も、morphDeltaResource_ のインデックスの頂点へ戻す。いずれかの SubMesh が morphs_ を持つときだけ確保する。
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexMorphSourceResource_;

		/// [EN] Bindless SRV of morphDeltaResource_.
		/// [JP] morphDeltaResource_ の bindless SRV。
		Uint morphDeltaBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of vertexMorphSourceResource_.
		/// [JP] vertexMorphSourceResource_ の bindless SRV。
		Uint vertexMorphSourceBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of vertexResource_.
		/// [JP] vertexResource_ の bindless SRV。
		Uint vertexBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of skinVertexResource_.
		/// [JP] skinVertexResource_ の bindless SRV。
		Uint skinVertexBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of indexResource_.
		/// [JP] indexResource_ の bindless SRV。
		Uint indexBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of positionResource_, read by the morph and skin blend passes.
		/// [JP] positionResource_ の bindless SRV。モーフとスキンのブレンドパスが読む。
		Uint positionBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of raytracingSkinVertexResource_, read by the skin blend pass.
		/// [JP] raytracingSkinVertexResource_ の bindless SRV。スキンのブレンドパスが読む。
		Uint raytracingSkinVertexBufferIndex_ = SC_INVALID;

		/// [EN] Bindless SRV of raytracingMorphDeltaResource_, read by the morph blend pass.
		/// [JP] raytracingMorphDeltaResource_ の bindless SRV。モーフのブレンドパスが読む。
		Uint raytracingMorphDeltaBufferIndex_ = SC_INVALID;

		/// [EN] Number of indices in indexResource_ (three per triangle).
		/// [JP] indexResource_ のインデックスの数(三角形1つにつき3つ)。
		Uint32 triangleIndexCount_ = 0;

		/// [EN] Number of vertices in vertexResource_ and positionResource_.
		/// [JP] vertexResource_ と positionResource_ の頂点の数。
		Uint32 proxyVertexCount_ = 0;

		/// [EN] Whether the RT proxy's non-finite-position fallback has been logged, so it is reported once per Crister rather than once per degenerate vertex (see Upload()'s RT proxy geometry loop).
		/// [JP] RT プロキシの非有限位置の代替処理をログに出したか。退化した頂点ごとではなく Crister ごとに1度だけ出すため(Upload() の RT プロキシジオメトリのループ参照)。
		Bool degenerateRaytracingPositionLogged_ = false;

		/// [EN] Shared LOD 0 vertex pool page, referenced by the LOD 0 cluster pages.
		/// [JP] 共有 LOD 0 頂点プールのページ。LOD 0 のクラスタページが参照する。
		Microsoft::WRL::ComPtr<ID3D12Resource> poolResource_;

		/// [EN] Bindless SRV of poolResource_.
		/// [JP] poolResource_ の bindless SRV。
		Uint poolBufferIndex_ = SC_INVALID;

		/// [EN] End (exclusive) of the vertices LOD 0 clusters reference; the pool covers [0, poolVertexEnd_). 0 when there are none.
		/// [JP] LOD 0 のクラスタが参照する頂点の終わり(含まない)。プールは [0, poolVertexEnd_) を持つ。無ければ 0。
		Uint32 poolVertexEnd_ = 0;

		/// [EN] Number of resident cluster pages reading the pool; the pool is evicted only at 0.
		/// [JP] プールを読んでいる常駐中のクラスタページの数。0 のときだけプールを追い出す。
		Uint32 poolResidentReferences_ = 0;

		/// [EN] Whether the pool is currently in VRAM.
		/// [JP] プールが今 VRAM にあるか。
		Bool poolResident_ = false;

		/// [EN] Whether the pool must never be evicted, because a pinned LOD 0 cluster reads it.
		/// [JP] ピン留めされた LOD 0 のクラスタが読むため、プールを追い出してはいけないか。
		Bool poolPinned_ = false;

		/// [EN] VRAM the pool takes while resident, counted against the geometry budget.
		/// [JP] 常駐中にプールが使う VRAM。ジオメトリの予算に数える。
		Uint64 poolSizeBytes_ = 0;

		/// [EN] Last frame a page reading the pool was drawn.
		/// [JP] プールを読むページが最後に描画されたフレーム。
		Uint64 poolLastUsedFrame_ = 0;

		/// [EN] Streaming page of each Cluster, index-aligned with clusters_.
		/// [JP] 各 Cluster のストリーミングページ。clusters_ とインデックスが揃っている。
		DynamicArray<StreamingGeometry> streamingGeometry_;

		/// [EN] Streaming state of each Bitmap, index-aligned with bitmaps_.
		/// [JP] 各 Bitmap のストリーミング状態。bitmaps_ とインデックスが揃っている。
		DynamicArray<StreamingTexture> streamingTextures_;

		/// [EN] Set whenever a texture's resident bindless index changes (MakeTextureMipResident/EvictTextureMip). The reflection/GI material table (RaytracingRenderer::BuildReflectionMaterialTable) caches TextureBindlessIndex() when it is built, so after a mip swap its entries point at freed or reused slots; MaterialsDirty() lets the renderer notice and rebuild it, and ClearMaterialsDirty() resets the flag.
		/// [JP] テクスチャの常駐中の bindless インデックスが変わるたびに立てる(MakeTextureMipResident/EvictTextureMip)。反射/GI のマテリアルテーブル(RaytracingRenderer::BuildReflectionMaterialTable)は構築時に TextureBindlessIndex() をキャッシュするので、ミップの入れ替え後は解放済みか再利用されたスロットを指してしまう。MaterialsDirty() でレンダラーがそれに気付いて再構築し、ClearMaterialsDirty() でフラグを戻す。
		mutable Bool materialsDirty_ = false;

		/// [EN] Device captured at Upload() so pages can stream in and out later; outlives every Crister.
		/// [JP] 後からページを出し入れできるよう Upload() で保持するデバイス。全ての Crister より長く生きる。
		ID3D12Device* device_ = nullptr;

		/// [EN] Queue the streaming uploads are submitted to, captured at Upload(); outlives every Crister.
		/// [JP] ストリーミングのアップロードを投入するキュー。Upload() で保持する。全ての Crister より長く生きる。
		D3D12CommandQueue* uploadQueue_ = nullptr;

		/// [EN] Heap the streamed pages and mips take their descriptors from, captured at Upload(); outlives every Crister.
		/// [JP] ストリーミングするページとミップがディスクリプタを取るヒープ。Upload() で保持する。全ての Crister より長く生きる。
		BindlessHeap* bindlessHeap_ = nullptr;

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

		/// [EN] Per-SubMesh model-space placements (see SubMeshPlacement), indexed like subMeshes_. Resolved from nodes_ in Upload() and not serialized.
		/// [JP] SubMesh ごとのモデル空間配置(SubMeshPlacement 参照)。subMeshes_ と同じインデックス。Upload() で nodes_ から解決し、シリアライズしない。
		DynamicArray<DynamicArray<Matrix>> subMeshPlacement_;

		/// [EN] Minimum corner of the placed model-space bounds (see PlacedPositionMin).
		/// [JP] 配置後のモデル空間での境界の最小コーナー(PlacedPositionMin 参照)。
		Vector3 placedPositionMin_ = { 0,0,0 };

		/// [EN] Extent of the placed model-space bounds (see PlacedPositionExtent).
		/// [JP] 配置後のモデル空間での境界の大きさ(PlacedPositionExtent 参照)。
		Vector3 placedPositionExtent_ = { 1,1,1 };

		/// [EN] Every live Crister that has uploaded, walked by the budget eviction. Streaming bookkeeping is shared by every Crister and touched only on the render thread; geometry and textures keep separate totals and budgets but share this registry and the eviction age guard.
		/// [JP] アップロード済みで生存中の全 Crister。予算による追い出しがこれを順に見る。ストリーミング管理は全 Crister で共有し、レンダースレッドからだけ触る。ジオメトリとテクスチャは合計と予算を別々に持つが、このレジストリと追い出しの経過フレームガードは共有する。
		static inline DynamicArray<Crister*> streamingRegistry_;

		/// [EN] VRAM taken by every resident geometry page and pool, across all Crister.
		/// [JP] 全 Crister の常駐中のジオメトリページとプールが使う VRAM の合計。
		static inline Uint64 totalResidentGeometryBytes_ = 0;

		/// [EN] VRAM taken by every resident texture mip, across all Crister.
		/// [JP] 全 Crister の常駐中のテクスチャミップが使う VRAM の合計。
		static inline Uint64 totalResidentTextureBytes_ = 0;

		/// [EN] VRAM the resident geometry may take before EvictClusterBudget starts evicting.
		/// [JP] EvictClusterBudget が追い出しを始めるまでに、常駐ジオメトリが使ってよい VRAM。
		static inline Uint64 geometryBudgetBytes_ = 512ull * 1024 * 1024;

		/// [EN] VRAM the resident texture mips may take before EvictTextureBudget starts evicting.
		/// [JP] EvictTextureBudget が追い出しを始めるまでに、常駐テクスチャミップが使ってよい VRAM。
		static inline Uint64 textureBudgetBytes_ = 256ull * 1024 * 1024;

		/// [EN] Frames a page or mip must go unused before it may be evicted, so work still in flight on the GPU never loses a resource it reads.
		/// [JP] ページやミップを追い出せるようになるまでに、使われないまま経つべきフレーム数。GPU でまだ実行中の処理が、読んでいるリソースを失わないようにするため。
		SC_CONSTEX Uint64 evictAgeFrames_ = 8;
	};
}