#include <GraphicsEngine/Model/Crister/CristerExtractor.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
	/**
	* [EN]
	* Flattens the Crister's triangles into a position/index pair, for
	* MeshCollisionLoader to bake into a ".collision" file. Exact takes
	* each SubMesh's LOD 0 cluster and Proxy its coarsest one; a static
	* SubMesh is emitted once per node placement, and vertices are
	* deduplicated within each placement. Positions are dequantised
	* with the same math as the shader decode. Returns false when no
	* triangle comes out.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister の三角形を位置/インデックス対へ展開する。
	* MeshCollisionLoader がこれを ".collision" ファイルへ焼き込む。
	* Exact は各 SubMesh の LOD 0 クラスタを、Proxy は最も粗い
	* クラスタを使う。静的な SubMesh はノードの配置ごとに1回ずつ出力し、
	* 頂点の重複排除は配置ごとに行う。位置はシェーダーのデコードと同じ
	* 計算で逆量子化する。三角形が1つも出なければ false を返す。
	*/
	Bool CristerExtractor::Collision(const Crister& crister, MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outPositions.clear();
		outIndices.clear();

		if (crister.geometry_.compressedVertices_.empty() || crister.geometry_.subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Dequantises positions against the Crister's AABB.
		/// [JP] Crister の AABB に対して位置を逆量子化する。
		CristerQuantizer quantizer;

		/// [EN] Maps a global (pre-dedup) vertex index to its position in outPositions, so triangles sharing a vertex reuse the same compact index instead of duplicating the position.
		/// [JP] グローバル（重複排除前）の頂点インデックスを outPositions 内の位置へ対応付ける。頂点を共有する三角形が同じコンパクトインデックスを再利用し、位置が重複しないようにする。
		std::unordered_map<Uint32, Uint32> remap;

		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			/// [EN] A SubMesh without clusters has no triangles.
			/// [JP] クラスタを持たない SubMesh には三角形が無い。
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] Exact takes the SubMesh's first (LOD 0, most detailed) cluster; Proxy takes its last (coarsest) cluster.
			/// [JP] Exact は SubMesh の最初のクラスタ（LOD 0、最も詳細）、Proxy は最後のクラスタ（最も粗い）を取る。
			Uint32 clusterIndex = detail == MeshCollisionDetail::Exact ? subMesh.clusterOffset_ : subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;

			if (clusterIndex >= crister.geometry_.clusters_.size())
			{
				continue;
			}

			/// [EN] Walk every meshlet in the chosen cluster, then every triangle corner in each meshlet, resolving each corner through vertexIndices_/primitiveIndices_ to a global vertex index.
			/// [JP] 選んだクラスタ内の全メシュレットを走査し、各メシュレット内の全三角形の各頂点を、vertexIndices_/primitiveIndices_ 経由でグローバル頂点インデックスへ解決する。
			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];

			/// [EN] A static SubMesh is placed once per node that references its mesh, as it is drawn; a skinned one, or one no node references, is taken as is.
			/// [JP] 静的な SubMesh は描画と同じく、そのメッシュを参照するノードごとに1回ずつ配置する。スキン付き、またはどのノードからも参照されないものはそのまま使う。
			DynamicArray<Matrix> placements;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				for (const Node& node : crister.scene_.nodes_)
				{
					if (node.mesh_ == subMesh.meshIndex_)
					{
						placements.push_back(node.globalTransform_);
					}
				}
			}
			if (placements.empty())
			{
				placements.push_back(Matrix::Identity);
			}

			/// [EN] Each placement is a separate copy of the triangles, so vertices are deduplicated within one placement only.
			/// [JP] 配置ごとに三角形の別のコピーになるので、頂点の重複排除は1つの配置の中だけで行う。
			for (const Matrix& placement : placements)
			{
				remap.clear();

				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];
							auto found = remap.find(globalIndex);
							if (found == remap.end())
							{
								/// [EN] First time this global vertex is seen: decode its position and append it, remembering the compact index for later corners that share it.
								/// [JP] このグローバル頂点を初めて見た場合: 位置をデコードして追加し、後で同じ頂点を共有する角のためにコンパクトインデックスを記憶する。
								Uint32 compactIndex = static_cast<Uint32>(outPositions.size());
								remap[globalIndex] = compactIndex;
								outPositions.push_back(Vector3::Transform(quantizer.DecodePosition(crister, crister.geometry_.compressedVertices_[globalIndex]), placement));
								outIndices.push_back(compactIndex);
							}
							else
							{
								/// [EN] Already emitted: reuse its compact index instead of pushing a duplicate position.
								/// [JP] 既に出力済み: 位置を重複追加せず、そのコンパクトインデックスを再利用する。
								outIndices.push_back(found->second);
							}
						}
					}
				}
			}
		}

		/// [EN] Succeeds only when at least one triangle came out.
		/// [JP] 三角形が1つ以上出たときだけ成功とする。
		return outIndices.size() >= 3;
	}

	/**
	* [EN]
	* Reads the Crister's LOD 0 geometry into full vertices
	* (position/normal/tangent/texcoord) and indices, in the same
	* global vertex index space compressedVertices_/vertexIndices_
	* use: outVertices is a 1:1 decoded copy of compressedVertices_
	* and outIndices reference it directly. A static SubMesh is moved
	* into the placement of the first node that references its mesh.
	* Because the ordering matches what the mesh shader reads, a
	* simulation running on these vertices can write vertex i straight
	* back into the render vertex buffer at index i. Softbody seeds its
	* render-side SoftbodyMesh from this. Returns false when no
	* triangle comes out.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister の LOD 0 ジオメトリを、compressedVertices_/vertexIndices_
	* と同じグローバル頂点インデックス空間のまま、フル頂点(位置/法線/
	* 接線/UV)とインデックスへ読み出す。outVertices は
	* compressedVertices_ を 1:1 でデコードしたコピーで、outIndices は
	* それを直接指す。静的な SubMesh は、そのメッシュを参照する最初の
	* ノードの配置へ移す。並びがメッシュシェーダーの読むものと同じなので、
	* これらの頂点で動くシミュレーションは頂点 i をそのまま描画用の
	* 頂点バッファのインデックス i へ書き戻せる。Softbody は描画側の
	* SoftbodyMesh をこれから作る。三角形が1つも出なければ false を返す。
	*/
	Bool CristerExtractor::FinestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outVertices.clear();
		outIndices.clear();

		if (crister.geometry_.compressedVertices_.empty() || crister.geometry_.subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Dequantises every vertex against the Crister's AABBs.
		/// [JP] Crister の AABB に対して全頂点を逆量子化する。
		CristerQuantizer quantizer;

		/// [EN] No remap: outVertices is a 1:1 decoded copy of compressedVertices_, so a global vertex index can be used directly as an index into it.
		/// [JP] リマップは行わない: outVertices は compressedVertices_ を 1:1 でデコードしたコピーなので、グローバル頂点インデックスをそのままインデックスとして使える。
		outVertices.reserve(crister.geometry_.compressedVertices_.size());
		for (const CompressedVertex& compressed : crister.geometry_.compressedVertices_)
		{
			outVertices.push_back(quantizer.DecodeVertex(crister, compressed));
		}

		/// [EN] Tracks which vertices have already been moved to their node placement, so a vertex shared by several triangles is transformed once.
		/// [JP] どの頂点をノード配置へ移し終えたかを記録し、複数の三角形が共有する頂点を1回だけ変換する。
		DynamicArray<Bool> placedVertices(outVertices.size(), false);
		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] LOD 0 is always the SubMesh's first cluster.
			/// [JP] LOD 0 は常に SubMesh の最初のクラスタ。
			Uint32 clusterIndex = subMesh.clusterOffset_;
			if (clusterIndex >= crister.geometry_.clusters_.size())
			{
				continue;
			}

			/// [EN] A static SubMesh takes the transform of the first node that references its mesh; a skinned one stays in bind pose.
			/// [JP] 静的な SubMesh は、そのメッシュを参照する最初のノードの変換を使う。スキン付きのものはバインドポーズのまま。
			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(crister.scene_.nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != crister.scene_.nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			/// [EN] Normals go through the inverse transpose so non-uniform scale keeps them perpendicular, and a mirroring placement flips the tangent handedness.
			/// [JP] 法線は逆転置行列で変換し、非一様スケールでも面に垂直なままにする。鏡映する配置では接線の利き手を反転する。
			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];

						/// [EN] The index is the global vertex index itself, since outVertices mirrors compressedVertices_.
						/// [JP] outVertices は compressedVertices_ と同じ並びなので、インデックスはグローバル頂点インデックスそのもの。
						outIndices.push_back(globalIndex);

						/// [EN] Move position, normal and tangent into the placement the first time the vertex is reached.
						/// [JP] その頂点に初めて来たときに、位置・法線・接線を配置へ移す。
						if (placed && !placedVertices[globalIndex])
						{
							placedVertices[globalIndex] = true;
							Vertex& vertex = outVertices[globalIndex];
							vertex.position_ = Vector3::Transform(vertex.position_, placement);
							vertex.normal_ = Vector3::TransformNormal(vertex.normal_, normalPlacement);
							vertex.normal_.Normalize();
							Vector3 tangent = Vector3::TransformNormal(Vector3(vertex.tangent_.x, vertex.tangent_.y, vertex.tangent_.z), placement);
							tangent.Normalize();
							vertex.tangent_ = Vector4(tangent.x, tangent.y, tangent.z, vertex.tangent_.w * tangentSign);
						}
					}
				}
			}
		}

		/// [EN] Succeeds only when at least one triangle came out.
		/// [JP] 三角形が1つ以上出たときだけ成功とする。
		return outIndices.size() >= 3;
	}

	/**
	* [EN]
	* Reads each SubMesh's coarsest cluster (the same range as
	* Collision's Proxy detail) into full vertices and indices,
	* compacted into its own local index space and welded by
	* position. A Softbody simulates this much smaller proxy mesh,
	* since a render-resolution mesh needs one constraint per edge
	* and is far beyond what a real-time soft body solver handles;
	* SoftbodyMesh binds each FinestVertices vertex to its nearest
	* proxy vertices and blends their simulated displacement back onto
	* the full-resolution mesh every frame. Returns false when no
	* triangle comes out.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各 SubMesh の最も粗いクラスタ(Collision の Proxy と同じ範囲)を、
	* 自身のローカルなインデックス空間に詰め、位置で溶接したフル頂点と
	* インデックスへ読み出す。描画解像度のメッシュは辺ごとに拘束が
	* 1つずつ要り、リアルタイムのソフトボディソルバーが扱える規模を
	* はるかに超えるため、Softbody はこのずっと小さいプロキシメッシュで
	* シミュレーションする。SoftbodyMesh が FinestVertices の各頂点を
	* 最も近いプロキシ頂点群へ結び付け、そのシミュレーション後の変位を
	* 毎フレームフル解像度のメッシュへ混ぜ戻す。三角形が1つも出なければ
	* false を返す。
	*/
	Bool CristerExtractor::CoarsestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outVertices.clear();
		outIndices.clear();

		if (crister.geometry_.compressedVertices_.empty() || crister.geometry_.subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Dequantises each vertex field against the Crister's AABBs.
		/// [JP] Crister の AABB に対して頂点の各フィールドを逆量子化する。
		CristerQuantizer quantizer;

		/// [EN] Maps a quantised bind-pose position to its slot in outVertices, so any two corners on the same position weld into one vertex even when they come from different global indices (UV-seam duplicates, or two indices LOD simplification left on the same spot). A soft body solver builds its constraints along edges, and a zero-length edge between two unwelded vertices has no direction to constrain. The 1e-4 world-unit grid is fine enough that distinct vertices never share a cell and coarse enough to absorb float round-trip noise between identical positions.
		/// [JP] 量子化したバインドポーズの位置を outVertices 内のスロットへ対応付け、同じ位置に落ちる角を、元のグローバルインデックスが違っていても(UV の継ぎ目で複製された頂点や、LOD の簡略化が同じ場所に残した2つのインデックス)1つの頂点へ溶接する。ソフトボディのソルバーは辺に沿って拘束を作り、溶接していない2頂点の間の長さ 0 の辺には拘束する向きが無い。1e-4 ワールド単位の格子は、別々の頂点が同じマスに入らない程度に細かく、同じ位置どうしの浮動小数の往復誤差を吸収できる程度に粗い。
		auto quantisedPositionKey = [](const Vector3& position) -> Uint64
		{
			constexpr Float gridScale = 10000.0f;
			Int64 x = static_cast<Int64>(std::lround(position.x * gridScale));
			Int64 y = static_cast<Int64>(std::lround(position.y * gridScale));
			Int64 z = static_cast<Int64>(std::lround(position.z * gridScale));

			/// [EN] Combine the three grid coordinates with large primes into one spatial hash key.
			/// [JP] 3つの格子座標を大きな素数で組み合わせ、1つの空間ハッシュのキーにする。
			Uint64 hash = static_cast<Uint64>(x) * 73856093ull;
			hash ^= static_cast<Uint64>(y) * 19349663ull;
			hash ^= static_cast<Uint64>(z) * 83492791ull;
			return hash;
		};

		std::unordered_map<Uint64, Uint32> remap;

		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] Coarsest cluster, the same range as Collision's Proxy detail.
			/// [JP] 最も粗いクラスタ。Collision の Proxy と同じ範囲。
			Uint32 clusterIndex = subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;
			if (clusterIndex >= crister.geometry_.clusters_.size())
			{
				continue;
			}

			/// [EN] A static SubMesh takes the transform of the first node that references its mesh; a skinned one stays in bind pose.
			/// [JP] 静的な SubMesh は、そのメッシュを参照する最初のノードの変換を使う。スキン付きのものはバインドポーズのまま。
			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(crister.scene_.nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != crister.scene_.nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			/// [EN] Normals go through the inverse transpose so non-uniform scale keeps them perpendicular, and a mirroring placement flips the tangent handedness.
			/// [JP] 法線は逆転置行列で変換し、非一様スケールでも面に垂直なままにする。鏡映する配置では接線の利き手を反転する。
			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];
						const CompressedVertex& compressed = crister.geometry_.compressedVertices_[globalIndex];

						Vertex vertex;

						/// [EN] Only the position is decoded up front; it alone decides whether this corner welds onto an existing vertex.
						/// [JP] 先にデコードするのは位置だけ。既存の頂点へ溶接するかは位置だけで決まる。
						vertex.position_ = quantizer.DecodePosition(crister, compressed);
						if (placed)
						{
							vertex.position_ = Vector3::Transform(vertex.position_, placement);
						}

						Uint64 key = quantisedPositionKey(vertex.position_);
						auto found = remap.find(key);
						if (found == remap.end())
						{
							/// [EN] A new position: decode the remaining attributes, place them like the position, and append the vertex.
							/// [JP] 新しい位置なら、残りの属性をデコードして位置と同じように配置し、頂点を追加する。
							Uint32 compactIndex = static_cast<Uint32>(outVertices.size());
							remap[key] = compactIndex;

							vertex.normal_ = quantizer.DecodeOctahedralNormal(compressed.normal_);
							vertex.tangent_ = quantizer.DecodeTangent(compressed);
							vertex.texcoord_ = quantizer.DecodeTexcoord(crister, compressed);
							if (placed)
							{
								vertex.normal_ = Vector3::TransformNormal(vertex.normal_, normalPlacement);
								vertex.normal_.Normalize();
								Vector3 tangent = Vector3::TransformNormal(Vector3(vertex.tangent_.x, vertex.tangent_.y, vertex.tangent_.z), placement);
								tangent.Normalize();
								vertex.tangent_ = Vector4(tangent.x, tangent.y, tangent.z, vertex.tangent_.w * tangentSign);
							}
							outVertices.push_back(vertex);

							outIndices.push_back(compactIndex);
						}
						else
						{
							/// [EN] A position already emitted: weld onto that vertex.
							/// [JP] 既に出力した位置なら、その頂点へ溶接する。
							outIndices.push_back(found->second);
						}
					}
				}
			}
		}

		/// [EN] Succeeds only when at least one triangle came out.
		/// [JP] 三角形が1つ以上出たときだけ成功とする。
		return outIndices.size() >= 3;
	}
}
