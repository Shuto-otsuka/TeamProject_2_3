#include <GraphicsEngine/Model/Crister/CristerFlattener.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	/**
	* [EN]
	* Builds the RT proxy from crister, placing each SubMesh by
	* subMeshPlacements (indexed like the SubMeshes), and records each
	* SubMesh's proxy vertex, triangle and morph delta ranges in it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* crister から RT プロキシを作る。各 SubMesh は subMeshPlacements
	* (SubMesh と同じインデックス)で配置し、各 SubMesh のプロキシの
	* 頂点・三角形・モーフデルタの範囲を SubMesh に記録する。
	*/
	void CristerFlattener::Flatten(Crister& crister, std::span<const DynamicArray<Matrix>> subMeshPlacements)
	{
		/// [EN] RT proxy geometry: flatten each SubMesh's cluster (see below for which), once per node placement, into a compact vertex set + 32-bit triangle list. Positions are decoded on the CPU with the same math as the shader decode.
		/// [JP] RT プロキシジオメトリ: 各 SubMesh のクラスタ(どれを使うかは下を参照)を、ノード配置ごとにコンパクトな頂点集合 + 32bit 三角形リストへ展開する。位置はシェーダーのデコードと同じ計算で CPU でデコードする。
		triangleIndices_.clear();
		vertices_.clear();
		positions_.clear();
		skinVertices_.clear();
		morphDeltas_.clear();

		CristerQuantizer quantizer;
		Bool degeneratePositionLogged = false;

		/// [EN] Per-vertex skin attributes for the RT proxy are built only for a model with exactly one skin, the case the proxy skin blend handles.
		/// [JP] RT プロキシの頂点ごとのスキン属性は、スキンがちょうど1つのモデルにだけ作る。プロキシのスキンブレンドが扱うのはその場合。
		skinVerticesBuilt_ = crister.scene_.skins_.size() == 1 && !crister.geometry_.compressedSkinVertices_.empty();
		std::unordered_map<Uint32, Uint32> raytracingRemap;
		for (Size subMeshIndex = 0; subMeshIndex < crister.geometry_.subMeshes_.size(); subMeshIndex++)
		{
			SubMesh& subMesh = crister.geometry_.subMeshes_[subMeshIndex];

			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] RT uses LOD 0 (full detail): reflections show the mesh at its best quality, at the cost of keeping the RT flat-index / position buffers fully resident. Rasterisation still streams.
			/// [JP] RT は LOD 0（フル詳細）を使う: 反射は最高品質のメッシュで描かれる。代償として RT のフラットインデックス/位置バッファは常に全量常駐する。ラスタ側のストリーミングは従来どおり。
			const Cluster& cluster = crister.geometry_.clusters_[subMesh.clusterOffset_];

			/// [EN] Encounter order for this SubMesh's compact vertices is contiguous ascending indices into vertices_ (globalIndex never collides across SubMeshes, since each owns a disjoint range of vertices_) — so [raytracingVertexOffset_, raytracingVertexOffset_ + raytracingVertexCount_) below is valid, and submeshMorphDeltas[target] fills in that same order.
			/// [JP] この SubMesh のコンパクト頂点の出現順は vertices_ への連続した昇順インデックスになる(globalIndex は SubMesh間で衝突しない、各 SubMesh が vertices_ の互いに素な範囲を持つため) — そのため下の [raytracingVertexOffset_, raytracingVertexOffset_ + raytracingVertexCount_) は正しく、submeshMorphDeltas[target] も同じ順序で埋まる。
			subMesh.raytracingVertexOffset_ = static_cast<Uint32>(vertices_.size());
			subMesh.raytracingTriangleOffset_ = static_cast<Uint32>(triangleIndices_.size() / 3);
			DynamicArray<DynamicArray<Vector3>> submeshMorphDeltas(subMesh.morphs_.size());

			const DynamicArray<Matrix>& placements = subMeshPlacements[subMeshIndex];

			/// [EN] A morphing SubMesh is flattened for its first placement only, because its morph deltas are laid out over a single copy of its vertices.
			/// [JP] モーフする SubMesh は最初の配置ぶんだけ展開する。モーフデルタはその頂点の1コピーぶんに対して並んでいるため。
			Size placementCount = subMesh.morphs_.empty() ? placements.size() : 1;
			for (Size placementIndex = 0; placementIndex < placementCount; placementIndex++)
			{
				const Matrix& placement = placements[placementIndex];
				Bool placed = placement != Matrix::Identity;
				Matrix normalPlacement = placement.Invert().Transpose();
				Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

				/// [EN] Each placement is its own copy of the vertices, so deduplication restarts per placement.
				/// [JP] 配置ごとに頂点の別のコピーになるので、重複排除は配置ごとにやり直す。
				raytracingRemap.clear();

				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = crister.geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = crister.geometry_.vertexIndices_[meshlet.vertexOffset_ + crister.geometry_.primitiveIndices_[byteOffset + corner]];
							auto found = raytracingRemap.find(globalIndex);
							if (found == raytracingRemap.end())
							{
								/// [EN] First time this vertex appears in this placement: give it the next compact index.
								/// [JP] この配置でこの頂点が初めて出たので、次のコンパクトインデックスを割り当てる。
								Uint32 compactIndex = static_cast<Uint32>(vertices_.size());
								raytracingRemap[globalIndex] = compactIndex;

								const CompressedVertex& compressed = crister.geometry_.compressedVertices_[globalIndex];

								/// [EN] A source asset with a degenerate/non-finite quantisation AABB (positionMin_/positionExtent_) decodes to a NaN/Inf position here even though the compressed bit pattern itself is well-formed. That NaN reaches the BLAS's position buffer, producing a NaN bounding box the traversal hardware can never cull against - every ray descends into it instead of skipping it, which is a no-page-fault DispatchRays hang rather than a crash. Falling back to the origin keeps this mesh's triangle count/indexing intact (so BLAS build still succeeds) while giving the BVH a finite, if wrong-looking, box.
								/// [JP] 量子化 AABB(positionMin_/positionExtent_)が退化/非有限なソースアセットは、圧縮ビットパターン自体は正常でもここで NaN/Inf の位置へデコードされる。その NaN は BLAS の位置バッファへそのまま渡り、走査ハードウェアが絶対にカリングできない NaN境界ボックスを生む - 全てのレイがスキップされずそこへ降りていくため、クラッシュではなくページフォルト無しの DispatchRays ハングになる。原点へフォールバックすれば、このメッシュの三角形数/インデックス構造は保ったまま(BLAS 構築を成功させたまま)、見た目はおかしくとも有限な境界ボックスを BVH に与えられる。
								Vector3 position = quantizer.DecodePosition(crister, compressed);
								if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
								{
									if (!degeneratePositionLogged)
									{
										SC_LOG_WARNING("RT プロキシの頂点位置が非有限です。ソースアセットの量子化 AABB が壊れている可能性があります。");
										degeneratePositionLogged = true;
									}
									position = Vector3::Zero;
								}

								/// [EN] A placed copy is decoded, moved into the placement (normals by the inverse transpose, tangent handedness flipped under a mirror) and re-encoded; an unplaced one is copied as is.
								/// [JP] 配置するコピーはデコードして配置へ移し(法線は逆転置行列で、鏡映では接線の利き手を反転)、符号化し直す。配置しないものはそのままコピーする。
								if (placed)
								{
									Vertex placedVertex = quantizer.DecodeVertex(crister, compressed);
									placedVertex.position_ = Vector3::Transform(position, placement);
									placedVertex.normal_ = Vector3::TransformNormal(placedVertex.normal_, normalPlacement);
									placedVertex.normal_.Normalize();
									Vector3 placedTangent = Vector3::TransformNormal(Vector3(placedVertex.tangent_.x, placedVertex.tangent_.y, placedVertex.tangent_.z), placement);
									placedTangent.Normalize();
									placedVertex.tangent_ = Vector4(placedTangent.x, placedTangent.y, placedTangent.z, placedVertex.tangent_.w * tangentSign);
									position = placedVertex.position_;
									vertices_.push_back(quantizer.EncodeVertex(placedVertex, crister.geometry_.positionMin_, crister.geometry_.positionExtent_, crister.geometry_.texcoordMin_, crister.geometry_.texcoordExtent_));
								}
								else
								{
									vertices_.push_back(compressed);
								}

								/// [EN] The float3 position goes to the BLAS buffer, and the corner references the new compact vertex.
								/// [JP] float3 の位置は BLAS 用のバッファへ入れ、角は新しいコンパクト頂点を指す。
								positions_.push_back(position);
								triangleIndices_.push_back(compactIndex);

								/// [EN] Keep the skin attributes index-aligned with the proxy vertices; unskinned SubMeshes get an empty entry.
								/// [JP] スキン属性をプロキシ頂点とインデックスを揃えて並べる。スキンの無い SubMesh には空のエントリを入れる。
								if (skinVerticesBuilt_)
								{
									if (subMesh.skinIndex_ >= 0 && globalIndex < crister.geometry_.compressedSkinVertices_.size())
									{
										skinVertices_.push_back(crister.geometry_.compressedSkinVertices_[globalIndex]);
									}
									else
									{
										skinVertices_.push_back(CompressedSkinVertex{});
									}
								}

								/// [EN] Gather this vertex's delta for every target in the same compact order, moved into the placement like a direction.
								/// [JP] この頂点の全ターゲットのデルタを同じコンパクトな順で集め、方向と同じように配置へ移す。
								if (!subMesh.morphs_.empty() && globalIndex >= subMesh.vertexOffset_)
								{
									Uint32 localVertexIndex = globalIndex - subMesh.vertexOffset_;
									for (Size targetIndex = 0; targetIndex < subMesh.morphs_.size(); targetIndex++)
									{
										const DynamicArray<Vector3>& deltas = subMesh.morphs_[targetIndex].positionDeltas_;
										Vector3 delta = localVertexIndex < deltas.size() ? deltas[localVertexIndex] : Vector3::Zero;
										submeshMorphDeltas[targetIndex].push_back(placed ? Vector3::TransformNormal(delta, placement) : delta);
									}
								}
							}
							else
							{
								/// [EN] Vertex already emitted in this placement: reuse its compact index.
								/// [JP] この配置で既に出力した頂点なので、そのコンパクトインデックスを使い回す。
								triangleIndices_.push_back(found->second);
							}
						}
					}
				}
			}

			/// [EN] Record how many proxy vertices and triangles this SubMesh produced.
			/// [JP] この SubMesh が出したプロキシの頂点と三角形の数を記録する。
			subMesh.raytracingVertexCount_ = static_cast<Uint32>(vertices_.size()) - subMesh.raytracingVertexOffset_;
			subMesh.raytracingTriangleCount_ = static_cast<Uint32>(triangleIndices_.size() / 3) - subMesh.raytracingTriangleOffset_;

			/// [EN] Append this SubMesh's deltas to the proxy pool, one target after another.
			/// [JP] この SubMesh のデルタを、ターゲットごとに続けてプロキシのプールへ追加する。
			if (!subMesh.morphs_.empty())
			{
				subMesh.raytracingMorphDeltaOffset_ = static_cast<Uint32>(morphDeltas_.size());
				for (const DynamicArray<Vector3>& targetDeltas : submeshMorphDeltas)
				{
					morphDeltas_.insert(morphDeltas_.end(), targetDeltas.begin(), targetDeltas.end());
				}
			}
		}

		/// [EN] Bounds of the placed geometry, from the proxy positions; without a proxy they fall back to the quantisation AABB.
		/// [JP] 配置後のジオメトリの境界をプロキシの位置から求める。プロキシが無ければ量子化の AABB にする。
		placedPositionMin_ = crister.geometry_.positionMin_;
		placedPositionExtent_ = crister.geometry_.positionExtent_;
		if (!positions_.empty())
		{
			Vector3 placedMax = positions_.front();
			placedPositionMin_ = positions_.front();
			for (const Vector3& position : positions_)
			{
				placedPositionMin_ = Vector3::Min(placedPositionMin_, position);
				placedMax = Vector3::Max(placedMax, position);
			}
			placedPositionExtent_ = placedMax - placedPositionMin_;
		}
	}

	/**
	* [EN]
	* Proxy vertices in the 16-byte GPU format, read by the hit shaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ヒットシェーダーが読む、16 バイトの GPU フォーマットのプロキシ頂点。
	*/
	std::span<const CompressedVertex> CristerFlattener::Vertices()const
	{
		return vertices_;
	}

	/**
	* [EN]
	* The proxy vertices' positions as float3, the BLAS input.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プロキシ頂点の位置を float3 にしたもの。BLAS の入力。
	*/
	std::span<const Vector3> CristerFlattener::Positions()const
	{
		return positions_;
	}

	/**
	* [EN]
	* Flat 32-bit triangle indices into the proxy vertices, three per
	* triangle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プロキシ頂点を指すフラットな 32bit の三角形インデックス。三角形
	* 1つにつき3つ。
	*/
	std::span<const Uint32> CristerFlattener::TriangleIndices()const
	{
		return triangleIndices_;
	}

	/**
	* [EN]
	* Skin attributes index-aligned with the proxy vertices; filled only
	* when SkinVerticesBuilt() is true.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プロキシ頂点とインデックスが揃ったスキン属性。SkinVerticesBuilt()
	* が true のときだけ埋まる。
	*/
	std::span<const CompressedSkinVertex> CristerFlattener::SkinVertices()const
	{
		return skinVertices_;
	}

	/**
	* [EN]
	* Morph target deltas, laid out per target within each SubMesh's
	* proxy vertex range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各 SubMesh のプロキシ頂点の範囲の中で、ターゲットごとに並ぶ
	* モーフデルタ。
	*/
	std::span<const Vector3> CristerFlattener::MorphDeltas()const
	{
		return morphDeltas_;
	}

	/**
	* [EN]
	* Whether per-vertex skin attributes were built, which happens only
	* for a model with exactly one skin.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 頂点ごとのスキン属性を作ったか。スキンがちょうど1つのモデルの
	* ときだけ作る。
	*/
	Bool CristerFlattener::SkinVerticesBuilt()const
	{
		return skinVerticesBuilt_;
	}

	/**
	* [EN]
	* Minimum corner of the placed geometry's bounds; the quantisation
	* AABB when there is no proxy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 配置後のジオメトリの境界の最小コーナー。プロキシが無ければ量子化の
	* AABB。
	*/
	Vector3 CristerFlattener::PlacedPositionMin()const
	{
		return placedPositionMin_;
	}

	/**
	* [EN]
	* Extent of the placed geometry's bounds; the quantisation AABB when
	* there is no proxy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 配置後のジオメトリの境界の大きさ。プロキシが無ければ量子化の
	* AABB。
	*/
	Vector3 CristerFlattener::PlacedPositionExtent()const
	{
		return placedPositionExtent_;
	}
}