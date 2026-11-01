#include <GraphicsEngine/Model/Crister/CristerClusterizer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Cluster/QuadricErrorMetrics.h>

namespace SeedCore
{
	/**
	* [EN]
	* Generates meshlets and a multi-level LOD hierarchy for all SubMeshes.
	*
	* This is the core of the Mesh Shader rendering pipeline preparation:
	*
	* ┌────────────────────────────────────────────────────────────────────┐
	* │  MESHLET: A small group of triangles processed by one GPU thread  │
	* │           group (workgroup) in the Mesh Shader.                   │
	* │                                                                   │
	* │  Constraints:                                                     │
	* │    - Max 64 unique vertices per meshlet                           │
	* │    - Max 124 triangles per meshlet                                │
	* │                                                                   │
	* │  Why 64 / 124?                                                    │
	* │    - 64 vertices = the typical thread count in an MS workgroup    │
	* │    - 124 triangles × 3 = 372 indices → fits in shared memory	   ｜
	* │    - These limits come from D3D12 Mesh Shader best practices      │
	* ├────────────────────────────────────────────────────────────────────┤
	* │  MESHLET BOUND: Per-meshlet bounding sphere + normal cone for     │
	* │                 culling in the Amplification Shader (AS).         │
	* │                                                                   │
	* │  The AS can skip entire meshlets that are:                        │
	* │    - Outside the view frustum (bounding sphere test)              │
	* │    - Facing away from the camera (normal cone test)               │
	* ├────────────────────────────────────────────────────────────────────┤
	* │  LOD (Level of Detail): Multiple simplified versions of the mesh. │
	* │                                                                   │
	* │  LOD 0 = original (full detail)                                   │
	* │  LOD 1 = ~50% triangles                                           │
	* │  LOD 2 = ~25% triangles                                           │
	* │  ...                                                              │
	* │  LOD N = minimum triangles (< 4 stops the chain)                  │
	* │                                                                   │
	* │  Each LOD is a "Cluster" containing its own meshlets.             │
	* │  The AS selects the appropriate LOD based on screen-space size.   │
	* │                                                                   │
	* │  lodError is CUMULATIVE (not per-level delta) so the AS can       │
	* │  compare it directly against the screen-space error threshold.    │
	* └────────────────────────────────────────────────────────────────────┘
	*
	* Data flow:
	*
	*   vertexIndices_  →  per-SubMesh slice
	*                          │
	*                          ▼
	*                   LOD 0: buildMeshletsFromIndices → computeBounds → Cluster
	*                          │
	*                          ▼
	*                   QEM Simplify (50% reduction)
	*                          │
	*                          ▼
	*                   LOD 1: buildMeshletsFromIndices → computeBounds → Cluster
	*                          │
	*                          ▼
	*                   QEM Simplify (50% reduction)
	*                          │
	*                         ...
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全 SubMesh に対してメシュレットとマルチレベル LOD 階層を生成する。
	*
	* Mesh Shader レンダリングパイプライン準備のコア部分:
	*
	* ┌────────────────────────────────────────────────────────────────────┐
	* │  メシュレット: GPU の 1 スレッドグループ（ワークグループ）で処理   │
	* │               される小さな三角形グループ。                         │
	* │                                                                    │
	* │  制約:                                                             │
	* │    - メシュレットあたり最大 64 固有頂点                            │
	* │    - メシュレットあたり最大 124 三角形                             │
	* │                                                                    │
	* │  なぜ 64 / 124 か？                                                │
	* │    - 64 頂点 = MS ワークグループの典型的なスレッド数               │
	* │    - 124 三角形 × 3 = 372 インデックス → 共有メモリに収まる      │
	* │    - これらの制限は D3D12 Mesh Shader のベストプラクティスによる   │
	* ├────────────────────────────────────────────────────────────────────┤
	* │  メシュレットバウンド: Amplification Shader (AS) でのカリング用    │
	* │                       メシュレットごとの包囲球 + 法線コーン。      │
	* │                                                                    │
	* │  AS は以下のメシュレットを丸ごとスキップできる:                    │
	* │    - ビューフラスタムの外（包囲球テスト）                          │
	* │    - カメラに背を向けている（法線コーンテスト）                    │
	* ├────────────────────────────────────────────────────────────────────┤
	* │  LOD（詳細度レベル）: メッシュの複数の簡略化バージョン。           │
	* │                                                                    │
	* │  LOD 0 = オリジナル（最大詳細度）                                  │
	* │  LOD 1 = 三角形 ~50%                                               │
	* │  LOD 2 = 三角形 ~25%                                               │
	* │  ...                                                               │
	* │  LOD N = 最小三角形数（4 未満でチェーン停止）                      │
	* │                                                                    │
	* │  各 LOD は独自のメシュレットを含む「Cluster」である。              │
	* │  AS が画面空間サイズに基づいて適切な LOD を選択する。              │
	* │                                                                    │
	* │  lodError は累積（レベルごとのデルタではない）なので、AS が        │
	* │  画面空間エラー閾値と直接比較できる。                              │
	* └────────────────────────────────────────────────────────────────────┘
	*
	* データフロー:
	*
	*   vertexIndices_  →  SubMesh ごとのスライス
	*                          │
	*                          ▼
	*                   LOD 0: buildMeshletsFromIndices → computeBounds → Cluster
	*                          │
	*                          ▼
	*                   QEM Simplify（50% 削減）
	*                          │
	*                          ▼
	*                   LOD 1: buildMeshletsFromIndices → computeBounds → Cluster
	*                          │
	*                          ▼
	*                   QEM Simplify（50% 削減）
	*                          │
	*                         ...
	*/
	void CristerClusterizer::BuildMeshlets(Crister& crister)const
	{
		/// [EN] Maximum LOD depth. 24 levels at 50% reduction per level gives a theoretical reduction of 2^24 ≈ 16 million : 1.
		/// [JP] 最大 LOD 深度。レベルごと 50% 削減で 24 レベルなら理論上 2^24 ≈ 1600 万 : 1 の削減。
		constexpr Uint32 maxLodLevels = 24;

		/// [EN] Move the original global index buffer out — we rebuild it from scratch as meshlet vertex indices (vertexIndices_) and primitive indices (primitiveIndices_).
		/// [JP] 元のグローバルインデックスバッファを退避 — メシュレットの頂点インデックス(vertexIndices_) とプリミティブインデックス (primitiveIndices_) として一から再構築する。
		DynamicArray<Uint32> originalIndices = std::move(crister.geometry_.vertexIndices_);
		crister.geometry_.vertexIndices_.clear();
		crister.geometry_.primitiveIndices_.clear();
		crister.geometry_.meshlets_.clear();
		crister.geometry_.meshletBounds_.clear();
		crister.geometry_.clusters_.clear();

		/**
		* [EN]
		* Packs a flat triangle index buffer (global vertex indices) into
		* meshlets with Meshlet and appends them to the Crister. Meshlet
		* places each meshlet at the start of its own tables, so the vertex
		* and triangle offsets are shifted by how much the Crister already
		* holds. Only the primitive index bytes the meshlets use are copied:
		* primitiveIndices_ stays a tight run of three bytes per triangle,
		* which the winding flip and page upload read as such.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フラットな三角形インデックスバッファ（全体での頂点番号）を Meshlet で
		* メッシュレットに詰め、Crister へ追加する。Meshlet は自分の表の先頭から
		* 並べるので、頂点と三角形の開始位置は Crister がすでに持っている分だけ
		* ずらす。三角形番号はメッシュレットが使うバイトだけを写す。
		* primitiveIndices_ は三角形1つにつき3バイトが隙間なく並ぶ列のまま保ち、
		* 向きの反転やページのアップロードはそれを前提に読む。
		*/
		auto buildMeshletsFromIndices = [&](const Uint32* indices, Size indexCount) -> void
			{
				Meshlet meshlet(std::span<const Uint32>(indices, indexCount));
				std::span<const MeshletDesc> meshlets = meshlet.Meshlets();

				Uint32 vertexBase = static_cast<Uint32>(crister.geometry_.vertexIndices_.size());
				Uint32 triangleBase = static_cast<Uint32>(crister.geometry_.primitiveIndices_.size());
				for (const MeshletDesc& source : meshlets)
				{
					MeshletDesc& destination = crister.geometry_.meshlets_.emplace_back(source);
					destination.vertexOffset_ += vertexBase;
					destination.triangleOffset_ += triangleBase;
				}

				std::span<const Uint32> vertexIndices = meshlet.VertexIndices();
				crister.geometry_.vertexIndices_.insert(crister.geometry_.vertexIndices_.end(), vertexIndices.begin(), vertexIndices.end());

				/// [EN] Leave out the padding Meshlet adds after the last triangle.
				/// [JP] Meshlet が最後の三角形の後ろに足す詰め物は写さない。
				Size usedByteCount = meshlets.empty() ? 0 : static_cast<Size>(meshlets.back().triangleOffset_) + static_cast<Size>(meshlets.back().triangleCount_) * 3;
				std::span<const Uint8> primitiveIndices = meshlet.PrimitiveIndices().first(usedByteCount);
				crister.geometry_.primitiveIndices_.insert(crister.geometry_.primitiveIndices_.end(), primitiveIndices.begin(), primitiveIndices.end());
			};

		/**
		* [EN]
		* Computes the bounding sphere and normal cone for a range of meshlets.
		*
		* Bounding sphere:
		*   - Center = average of all vertex positions in the meshlet.
		*   - Radius = max distance from center to any vertex.
		*   Used by the AS for frustum culling (reject meshlets entirely
		*   outside the camera frustum).
		*
		* Normal cone:
		*   - coneAxis = average of all face normals in the meshlet.
		*   - coneCutoff = minimum dot product between any face normal and
		*     the cone axis. If coneCutoff > 0, all faces point roughly
		*     the same direction, and the AS can do backface culling:
		*     the entire meshlet faces away from the camera when
		*     dot(center - camera, coneAxis) >= sqrt(1 - coneCutoff^2) *
		*     length(center - camera) + radius (coneCutoff is the cosine of
		*     the cone's half-angle, the test needs its sine).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メシュレット範囲に対して包囲球と法線コーンを計算する。
		*
		* 包囲球:
		*   - 中心 = メシュレット内の全頂点位置の平均。
		*   - 半径 = 中心からいずれかの頂点までの最大距離。
		*   AS がフラスタムカリングに使用する（カメラフラスタムの完全に
		*   外にあるメシュレットを棄却）。
		*
		* 法線コーン:
		*   - coneAxis = メシュレット内の全面法線の平均。
		*   - coneCutoff = いずれかの面法線とコーン軸の最小内積。
		*     coneCutoff > 0 なら全面がほぼ同じ方向を向いており、
		*     AS がバックフェイスカリングを行える:
		*     dot(center - camera, coneAxis) >= sqrt(1 - coneCutoff^2) *
		*     length(center - camera) + radius ならメシュレット全体が
		*     カメラに背を向けている(coneCutoff はコーン半角の cos で、
		*     判定にはその sin を使う)。
		*/
		auto computeBounds = [&](Uint32 meshletBegin, Uint32 meshletEnd)
			{
				for (Uint32 meshletIndex = meshletBegin; meshletIndex < meshletEnd; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					MeshletBound& bound = crister.geometry_.meshletBounds_.emplace_back();

					/// [EN] Step 1: Compute bounding sphere.
					/// [JP] ステップ 1: 包囲球を計算する。

					/// [EN] Compute the centroid (average position) of all vertices in this meshlet.
					/// [JP] このメシュレットの全頂点の重心（平均位置）を計算する。
					Vector3 center = Vector3::Zero;
					for (Uint32 index = 0; index < meshlet.vertexCount_; index++)
					{
						Uint32 vertexIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + index];
						center += crister.geometry_.vertices_[vertexIndex].position_;
					}
					center /= static_cast<Float>(meshlet.vertexCount_);

					/// [EN] Find the maximum squared distance from any vertex to the centroid.
					/// [JP] いずれかの頂点から重心までの最大二乗距離を求める。
					Float maxDistance2 = 0;
					for (Uint32 index = 0; index < meshlet.vertexCount_; index++)
					{
						Uint32 vertexIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + index];
						Vector3 difference = crister.geometry_.vertices_[vertexIndex].position_ - center;
						Float distance2 = difference.Dot(difference);
						if (distance2 > maxDistance2)
						{
							maxDistance2 = distance2;
						}
					}

					bound.center_ = center;
					bound.radius_ = std::sqrt(maxDistance2);

					/// [EN] Step 2: Compute normal cone.
					/// [JP] ステップ 2: 法線コーンを計算する。

					/// [EN] Compute the average face normal across all triangles in the meshlet.
					/// [JP] メシュレット内の全三角形にわたる平均面法線を計算する。
					Vector3 avgNormal = Vector3::Zero;
					Uint32 faceCount = 0;

					for (Uint32 triangle = 0; triangle < meshlet.triangleCount_; triangle++)
					{
						Uint32 base = meshlet.triangleOffset_ + triangle * 3;

						/// [EN] Resolve the 2-level indirection: primitiveIndices_ → vertexIndices_ → global vertex.
						/// [JP] 2 段階の間接参照を解決: primitiveIndices_ → vertexIndices_ → グローバル頂点。
						Uint32 gi0 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base]];
						Uint32 gi1 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 1]];
						Uint32 gi2 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 2]];

						Vector3 p0 = crister.geometry_.vertices_[gi0].position_;
						Vector3 p1 = crister.geometry_.vertices_[gi1].position_;
						Vector3 p2 = crister.geometry_.vertices_[gi2].position_;

						Vector3 e1 = p1 - p0;
						Vector3 e2 = p2 - p0;
						Vector3 n = e2.Cross(e1);
						Float length = n.Length();
						if (length < 1e-8f)
						{
							continue;
						}
						n /= length;

						avgNormal += n;
						faceCount++;
					}

					if (faceCount > 0)
					{
						Float length = avgNormal.Length();
						if (length > 1e-8f)
						{
							avgNormal /= length;
							bound.coneAxis_ = avgNormal;

							/// [EN] Find the tightest cone that contains all face normals: coneCutoff = min dot(faceNormal, avgNormal) over all faces. If this is > 0, all faces are within a hemisphere centered on avgNormal.
							/// [JP] 全面法線を包含する最小のコーンを求める:coneCutoff = 全面について min dot(faceNormal, avgNormal)。これが > 0 なら全面が avgNormal 中心の半球内にある。
							Float minDot = 1.0f;
							for (Uint32 triangle = 0; triangle < meshlet.triangleCount_; triangle++)
							{
								Uint32 base = meshlet.triangleOffset_ + triangle * 3;
								Uint32 gi0 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base]];
								Uint32 gi1 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 1]];
								Uint32 gi2 = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[base + 2]];

								Vector3 p0 = crister.geometry_.vertices_[gi0].position_;
								Vector3 p1 = crister.geometry_.vertices_[gi1].position_;
								Vector3 p2 = crister.geometry_.vertices_[gi2].position_;

								Vector3 e1 = p1 - p0;
								Vector3 e2 = p2 - p0;
								Vector3 n = e2.Cross(e1);
								Float length2 = n.Length();
								if (length2 < 1e-8f)
								{
									continue;
								}
								n /= length2;

								Float dot = n.Dot(avgNormal);
								if (dot < minDot)
								{
									minDot = dot;
								}
							}
							bound.coneCutoff_ = minDot;
						}
					}
				}
			};

		/**
		* [EN]
		* Detects "boundary" vertices in a triangle mesh — vertices that
		* touch an edge used by exactly one triangle (an open/unwelded
		* edge). QuadricErrorMetrics::Simplify has no concept of boundaries
		* on its own: without locking, it happily collapses boundary edges
		* like any interior one, since a boundary vertex's quadric is only
		* built from its (one-sided) adjacent triangle planes and has no
		* term discouraging movement along the open edge. For a mesh made
		* of many small, non-closed parts (e.g. separately-exported prop
		* pieces that are meant to butt against each other, as opposed to
		* a single watertight mesh), this lets QEM shrink/warp every part's
		* silhouette at LOD1+ — the parts cave inward as their boundaries
		* get pulled toward the interior, which reads as the whole model
		* collapsing into flattened shards. Locking boundary vertices (see
		* QuadricErrorMetrics::Simplify's `locked` parameter) keeps their
		* position fixed — an edge with both endpoints on the boundary
		* becomes uncollapsible, preserving the silhouette, while interior
		* edges can still collapse onto a boundary vertex to reduce
		* triangle count.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 三角形メッシュの「境界」頂点を検出する — ちょうど 1 つの三角形からしか
		* 使われていないエッジ（開いた/溶接されていないエッジ）に接する頂点。
		* QuadricErrorMetrics::Simplify はそれ自体では境界という概念を持たない:
		* ロックしなければ、内部のエッジと同じように境界エッジも平気で崩壊させる。
		* 境界頂点の Quadric は片側の隣接三角形平面からしか構築されず、開いた
		* エッジに沿った移動を抑止する項が無いためである。多数の小さく閉じていない
		* パーツ（単一のウォータータイトメッシュではなく、互いに突き合わさる想定で
		* 別々にエクスポートされたプロップパーツなど）からなるメッシュでは、これにより
		* LOD1 以降で各パーツのシルエットが縮小/歪曲する — 境界が内側に引っ張られて
		* パーツが陥没し、モデル全体が潰れた破片の集まりに見える。境界頂点をロックする
		* （QuadricErrorMetrics::Simplify の `locked` 引数を参照）ことで位置を固定でき、
		* 両端点が境界にあるエッジは崩壊不可能になりシルエットが保たれる一方、内部の
		* エッジは境界頂点へ向けて崩壊でき三角形数の削減は引き続き行える。
		*/
		/// [EN] Returns std::unique_ptr<Bool[]> rather than DynamicArray<Bool> (= std::vector<bool>) on purpose: vector<bool> is bit-packed and has no .data(), so it can't produce the raw `const Bool*` QuadricErrorMetrics::Simplify's `locked` parameter needs.
		/// [JP] DynamicArray<Bool>（= std::vector<bool>）ではなくstd::unique_ptr<Bool[]> を返すのは意図的: vector<bool> はビットパック実装で .data() を持たないため、QuadricErrorMetrics::Simplify の `locked` 引数が要求する生の `const Bool*` を作れない。
		auto computeBoundaryLocks = [](const Uint32* indices, Size indexCount, Size vertexCount) -> std::unique_ptr<Bool[]>
			{
				std::unordered_map<Uint64, Uint32> edgeUseCount;
				Size triangleCount = indexCount / 3;
				for (Size triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
				{
					Uint32 triangleVertices[3] = { indices[triangleIndex * 3], indices[triangleIndex * 3 + 1], indices[triangleIndex * 3 + 2] };
					for (Int edgeIndex = 0; edgeIndex < 3; edgeIndex++)
					{
						Uint32 vertexA = triangleVertices[edgeIndex];
						Uint32 vertexB = triangleVertices[(edgeIndex + 1) % 3];
						if (vertexA > vertexB)
						{
							std::swap(vertexA, vertexB);
						}
						Uint64 key = (static_cast<Uint64>(vertexA) << 32) | vertexB;
						edgeUseCount[key]++;
					}
				}

				/// [EN] make_unique<Bool[]>(n) value-initialises every element to false.
				/// [JP] make_unique<Bool[]>(n) は全要素を false に値初期化する。
				std::unique_ptr<Bool[]> locked = std::make_unique<Bool[]>(vertexCount);
				for (const auto& [key, count] : edgeUseCount)
				{
					if (count == 1)
					{
						locked[static_cast<Uint32>(key >> 32)] = true;
						locked[static_cast<Uint32>(key & 0xFFFFFFFF)] = true;
					}
				}
				return locked;
			};

		/// [EN] Process each SubMesh independently: generate LOD 0, then repeatedly simplify with QEM to build LOD 1, 2, ... up to maxLodLevels. Each LOD gets its own set of meshlets (a Cluster).
		/// [JP] 各 SubMesh を独立に処理: LOD 0 を生成した後、QEM で繰り返し簡略化して LOD 1, 2, ... を maxLodLevels まで構築する。各 LOD は独自のメシュレットセット（Cluster）を持つ。
		for (SubMesh& sub : crister.geometry_.subMeshes_)
		{
			sub.clusterOffset_ = static_cast<Uint32>(crister.geometry_.clusters_.size());

			/// [EN] Extract this SubMesh's index slice from the original (pre-meshlet) indices.
			/// [JP] 元の（メシュレット化前の）インデックスからこの SubMesh のインデックススライスを抽出する。
			DynamicArray<Uint32> currentIndices(originalIndices.begin() + sub.indexOffset_, originalIndices.begin() + sub.indexOffset_ + sub.indexCount_);

			/// [EN] Texel density for texture streaming (SubMesh::texcoordDensity_): sqrt(sum UV area / sum world area) over the full-detail triangles, i.e. UV units per mesh-local world unit. Area- weighted, so a few stretched/degenerate triangles don't skew it.
			/// [JP] テクスチャストリーミング用のテクセル密度(SubMesh::texcoordDensity_):フル詳細の三角形全体での sqrt(UV 面積合計 / ワールド面積合計)、すなわちメッシュローカル 1 ワールド単位あたりの UV 量。面積で重み付けするため、少数の引き伸ばされた/縮退した三角形に引きずられない。
			{
				Double worldAreaSum = 0.0;
				Double texcoordAreaSum = 0.0;
				for (Size corner = 0; corner + 2 < currentIndices.size(); corner += 3)
				{
					const Vertex& vertex0 = crister.geometry_.vertices_[currentIndices[corner + 0]];
					const Vertex& vertex1 = crister.geometry_.vertices_[currentIndices[corner + 1]];
					const Vertex& vertex2 = crister.geometry_.vertices_[currentIndices[corner + 2]];

					Vector3 edge1 = vertex1.position_ - vertex0.position_;
					Vector3 edge2 = vertex2.position_ - vertex0.position_;
					worldAreaSum += 0.5 * static_cast<Double>(edge1.Cross(edge2).Length());

					Vector2 texcoordEdge1 = vertex1.texcoord_ - vertex0.texcoord_;
					Vector2 texcoordEdge2 = vertex2.texcoord_ - vertex0.texcoord_;
					texcoordAreaSum += 0.5 * std::abs(static_cast<Double>(texcoordEdge1.x) * texcoordEdge2.y - static_cast<Double>(texcoordEdge2.x) * texcoordEdge1.y);
				}
				sub.texcoordDensity_ = (worldAreaSum > 1e-12 && texcoordAreaSum > 1e-12) ? static_cast<Float>(std::sqrt(texcoordAreaSum / worldAreaSum)) : 0.0f;
			}

			/**
			* [EN]
			* Build a local ↔ global vertex index mapping for this SubMesh.
			*
			* The indices in currentIndices are GLOBAL (into crister.vertices_).
			* QEM needs a DENSE local vertex array (0, 1, 2, ..., N-1), so we:
			*   1. Collect all unique global indices → localToGlobal
			*   2. Build the reverse map          → globalToLocal
			*   3. Extract local positions from crister.vertices_
			*   4. Remap the indices to local space
			*
			* After QEM simplifies, we map back to global to store the results.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* この SubMesh のローカル ↔ グローバル頂点インデックスマッピングを構築する。
			*
			* currentIndices 内のインデックスはグローバル（crister.vertices_ 内）。
			* QEM には密なローカル頂点配列 (0, 1, 2, ..., N-1) が必要なので:
			*   1. 全ユニークグローバルインデックスを収集 → localToGlobal
			*   2. 逆マップを構築                      → globalToLocal
			*   3. crister.vertices_ からローカル位置を抽出
			*   4. インデックスをローカル空間にリマップ
			*
			* QEM での簡略化後、結果を格納するためにグローバルに戻す。
			*/
			std::set<Uint32> usedSet(currentIndices.begin(), currentIndices.end());
			DynamicArray<Uint32> localToGlobal(usedSet.begin(), usedSet.end());
			std::unordered_map<Uint32, Uint32> globalToLocal;
			for (Uint32 index = 0; index < static_cast<Uint32>(localToGlobal.size()); index++)
			{
				globalToLocal[localToGlobal[index]] = index;
			}

			DynamicArray<Vector3> localPositions(localToGlobal.size());
			for (Size index = 0; index < localToGlobal.size(); index++)
			{
				localPositions[index] = crister.geometry_.vertices_[localToGlobal[index]].position_;
			}

			DynamicArray<Uint32> localIndices(currentIndices.size());
			for (Size index = 0; index < currentIndices.size(); index++)
			{
				localIndices[index] = globalToLocal[currentIndices[index]];
			}

			/// [EN] LOD 0: Original detail. Build meshlets from unmodified indices.
			/// [JP] LOD 0: オリジナル詳細度。未変更のインデックスからメシュレットを構築。
			Uint32 meshletBegin = static_cast<Uint32>(crister.geometry_.meshlets_.size());
			buildMeshletsFromIndices(currentIndices.data(), currentIndices.size());
			Uint32 meshletEnd = static_cast<Uint32>(crister.geometry_.meshlets_.size());
			computeBounds(meshletBegin, meshletEnd);

			Cluster& lod0 = crister.geometry_.clusters_.emplace_back();
			lod0.meshletOffset_ = meshletBegin;
			lod0.meshletCount_ = meshletEnd - meshletBegin;
			lod0.lodLevel_ = 0;
			lod0.lodError_ = 0.0f;

			/// [EN] State carried forward across LOD levels for iterative simplification.
			/// [JP] 反復的な簡略化のために LOD レベル間で持ち越す状態。
			DynamicArray<Vector3> lodPositions = localPositions;
			DynamicArray<Uint32> lodIndices = localIndices;

			/// [EN] lodVertexMap[localIndex] = global vertex index. Initially maps to the SubMesh's own vertices; updated each LOD as QEM creates new vertices.
			/// [JP] lodVertexMap[ローカルインデックス] = グローバル頂点インデックス。初期値は SubMesh 自身の頂点へのマップ。QEM が新頂点を作るたびに更新。
			DynamicArray<Uint32> lodVertexMap(localToGlobal.size());
			for (Uint32 index = 0; index < static_cast<Uint32>(localToGlobal.size()); index++)
			{
				lodVertexMap[index] = localToGlobal[index];
			}

			/// [EN] Cumulative error from LOD 0 to the current LOD. The AS uses this to decide whether this LOD is acceptable for the current screen-space pixel budget. It must be monotonically increasing so that LOD N+1 >= LOD N.
			/// [JP] LOD 0 から現在の LOD までの累積誤差。AS はこれを使って、この LOD が現在の画面空間ピクセル予算に対して許容可能かを判断する。LOD N+1 >= LOD N となるよう単調増加でなければならない。
			Float accumulatedError = 0.0f;

			/// [EN] LOD 1..N: Iteratively simplify with QEM.
			/// [JP] LOD 1..N: QEM で反復的に簡略化する。
			for (Uint32 lod = 1; lod < maxLodLevels; lod++)
			{
				Size currentTriangleCount = lodIndices.size() / 3;

				/// [EN] Target is 50% of the current triangle count. Stop if too few remain.
				/// [JP] 目標は現在の三角形数の 50%。残りが少なすぎたら停止。
				Size targetTriangleCount = currentTriangleCount / 2;
				if (targetTriangleCount < 4)
				{
					break;
				}

				/// [EN] Run QEM simplification on the current LOD's local mesh. Boundary vertices are recomputed every level (topology changes as edges collapse) and locked so open edges can't be pulled inward — see computeBoundaryLocks above.
				/// [JP] 現在の LOD のローカルメッシュに対して QEM 簡略化を実行する。境界頂点はレベルごとに再計算し（エッジ崩壊でトポロジーが変わるため）、開いたエッジが内側に引っ張られないようロックする— 上記 computeBoundaryLocks 参照。
				std::unique_ptr<Bool[]> boundaryLocks = computeBoundaryLocks(lodIndices.data(), lodIndices.size(), lodPositions.size());
				QuadricErrorMetrics qem;
				auto result = qem.Simplify(lodPositions.data(), lodPositions.size(), lodIndices.data(), lodIndices.size(), targetTriangleCount, boundaryLocks.get());

				/// [EN] If QEM couldn't reduce the mesh at all (e.g. all edges locked), stop.
				/// [JP] QEM がメッシュをまったく削減できなかった場合（全エッジがロックなど）、停止。
				if (result.indices.size() / 3 >= currentTriangleCount)
				{
					break;
				}

				/**
				* [EN]
				* Create new vertices for this LOD in crister.vertices_.
				*
				* QEM may have moved vertex positions to optimal locations, so we
				* can't reuse the original vertices. For each surviving vertex:
				*   1. Look up which original vertex it descended from (vertexMap).
				*   2. Copy that original vertex's attributes (normal, texcoord, etc.).
				*   3. Override the position with QEM's optimised position.
				*   4. Append as a new vertex at the end of crister.vertices_.
				*
				* This means each LOD level adds its own vertex copies. This is
				* intentional — the Mesh Shader will access the correct LOD's
				* vertices via the meshlet's vertexOffset_.
				*
				* ---------------------------------------------------------------------
				*
				* [JP]
				* この LOD の新しい頂点を crister.vertices_ に作成する。
				*
				* QEM は頂点位置を最適な場所に移動した可能性があるため、元の頂点は
				* 再利用できない。生き残った各頂点について:
				*   1. 元のどの頂点から派生したかを参照する (vertexMap)。
				*   2. その元の頂点の属性（法線、テクスチャ座標など）をコピーする。
				*   3. 位置を QEM の最適化された位置で上書きする。
				*   4. crister.vertices_ の末尾に新しい頂点として追加する。
				*
				* つまり各 LOD レベルが独自の頂点コピーを追加する。これは意図的で、
				* Mesh Shader がメシュレットの vertexOffset_ を通じて正しい LOD の
				* 頂点にアクセスする。
				*/
				Uint32 newVertexBase = static_cast<Uint32>(crister.geometry_.vertices_.size());
				crister.geometry_.vertices_.resize(newVertexBase + result.positions.size());
				crister.geometry_.vertexMorphSource_.resize(newVertexBase + result.positions.size());

				DynamicArray<Uint32> newLodVertexMap(result.vertexMap.size());
				for (Size index = 0; index < result.positions.size(); index++)
				{
					Uint32 originalLocal = result.vertexMap[index];
					Uint32 globalIndex = lodVertexMap[originalLocal];

					/// [EN] Copy all attributes from the original vertex, then replace position.
					/// [JP] 元の頂点から全属性をコピーし、位置だけ置き換える。
					Vertex vertex = crister.geometry_.vertices_[globalIndex];
					vertex.position_ = result.positions[index];
					crister.geometry_.vertices_[newVertexBase + index] = vertex;

					/// [EN] Propagate forward rather than pointing at globalIndex directly - globalIndex may itself be a previous LOD's duplicated vertex, and vertexMorphSource_[globalIndex] already resolves to the TRUE original ancestor in that case (see vertexMorphSource_'s comment).
					/// [JP] globalIndex を直接指すのではなく前方へ伝播する —globalIndex 自体が前の LOD の複製頂点である場合があり、その場合 vertexMorphSource_[globalIndex] は既に真のオリジナル祖先へ解決済み(vertexMorphSource_ のコメント参照)。
					crister.geometry_.vertexMorphSource_[newVertexBase + index] = crister.geometry_.vertexMorphSource_[globalIndex];
					newLodVertexMap[index] = newVertexBase + static_cast<Uint32>(index);
				}

				/// [EN] Remap QEM's local indices to global indices (into crister.vertices_).
				/// [JP] QEM のローカルインデックスをグローバルインデックス（crister.vertices_ 内）にリマップする。
				DynamicArray<Uint32> globalLodIndices(result.indices.size());
				for (Size index = 0; index < result.indices.size(); index++)
				{
					globalLodIndices[index] = newVertexBase + result.indices[index];
				}

				/// [EN] Build meshlets for this LOD level and compute their bounds.
				/// [JP] この LOD レベルのメシュレットを構築し、バウンドを計算する。
				Uint32 lodMeshletBegin = static_cast<Uint32>(crister.geometry_.meshlets_.size());
				buildMeshletsFromIndices(globalLodIndices.data(), globalLodIndices.size());
				Uint32 lodMeshletEnd = static_cast<Uint32>(crister.geometry_.meshlets_.size());
				computeBounds(lodMeshletBegin, lodMeshletEnd);

				/**
				* [EN]
				* Compute this LOD level's geometric error and accumulate it.
				*
				* For each vertex that survived QEM, measure how far it moved
				* from its pre-simplification position. The maximum displacement
				* across all vertices is this level's error contribution.
				*
				* We ACCUMULATE (not replace) because LOD error must be monotonically
				* increasing for the AS LOD selection to work correctly. If LOD 1
				* introduced 0.5 units of error and LOD 2 introduces 0.3 more,
				* the total error at LOD 2 is 0.8, not 0.3.
				*
				* ---------------------------------------------------------------------
				*
				* [JP]
				* この LOD レベルの幾何学的誤差を計算し、累積する。
				*
				* QEM を生き残った各頂点について、簡略化前の位置からどれだけ移動したかを
				* 測定する。全頂点にわたる最大変位がこのレベルの誤差寄与となる。
				*
				* AS の LOD 選択が正しく機能するには LOD 誤差が単調増加でなければ
				* ならないため、置換ではなく累積する。LOD 1 で 0.5 単位の誤差が
				* 導入され、LOD 2 でさらに 0.3 導入された場合、LOD 2 での
				* 総誤差は 0.3 ではなく 0.8 である。
				*/
				Float maxLevelError = 0.0f;
				for (Size index = 0; index < result.positions.size(); index++)
				{
					Uint32 originalLocal = result.vertexMap[index];
					Vector3 difference = result.positions[index] - lodPositions[originalLocal];
					Float error = difference.Length();
					if (error > maxLevelError)
					{
						maxLevelError = error;
					}
				}
				accumulatedError += maxLevelError;

				/// [EN] Create a Cluster for this LOD level.
				/// [JP] この LOD レベルの Cluster を作成する。
				Cluster& lodCluster = crister.geometry_.clusters_.emplace_back();
				lodCluster.meshletOffset_ = lodMeshletBegin;
				lodCluster.meshletCount_ = lodMeshletEnd - lodMeshletBegin;
				lodCluster.lodLevel_ = lod;
				lodCluster.lodError_ = accumulatedError;

				/// [EN] Carry state forward for the next LOD iteration.
				/// [JP] 次の LOD 反復のために状態を持ち越す。
				lodPositions = std::move(result.positions);
				lodIndices = std::move(result.indices);
				lodVertexMap = std::move(newLodVertexMap);
			}

			sub.clusterCount_ = static_cast<Uint32>(crister.geometry_.clusters_.size()) - sub.clusterOffset_;
		}
	}
}