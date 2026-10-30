#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>

namespace SeedCore
{
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
	Bool Crister::BakeCollision(MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outPositions.clear();
		outIndices.clear();

		if (geometry_.compressedVertices_.empty() || geometry_.subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Maps a global (pre-dedup) vertex index to its position in outPositions, so triangles sharing a vertex reuse the same compact index instead of duplicating the position.
		/// [JP] グローバル（重複排除前）の頂点インデックスを outPositions 内の位置へ対応付ける。頂点を共有する三角形が同じコンパクトインデックスを再利用し、位置が重複しないようにする。
		std::unordered_map<Uint32, Uint32> remap;

		for (const SubMesh& subMesh : geometry_.subMeshes_)
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

			if (clusterIndex >= geometry_.clusters_.size())
			{
				continue;
			}

			/// [EN] Walk every meshlet in the chosen cluster, then every triangle corner in each meshlet, resolving each corner through vertexIndices_/primitiveIndices_ to a global vertex index.
			/// [JP] 選んだクラスタ内の全メシュレットを走査し、各メシュレット内の全三角形の各頂点を、vertexIndices_/primitiveIndices_ 経由でグローバル頂点インデックスへ解決する。
			const Cluster& cluster = geometry_.clusters_[clusterIndex];

			/// [EN] A static SubMesh is placed once per node that references its mesh, as it is drawn; a skinned one, or one no node references, is taken as is.
			/// [JP] 静的な SubMesh は描画と同じく、そのメッシュを参照するノードごとに1回ずつ配置する。スキン付き、またはどのノードからも参照されないものはそのまま使う。
			DynamicArray<Matrix> placements;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				for (const Node& node : scene_.nodes_)
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
					const MeshletDesc& meshlet = geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = geometry_.vertexIndices_[meshlet.vertexOffset_ + geometry_.primitiveIndices_[byteOffset + corner]];
							auto found = remap.find(globalIndex);
							if (found == remap.end())
							{
								/// [EN] First time this global vertex is seen: decode its position and append it, remembering the compact index for later corners that share it.
								/// [JP] このグローバル頂点を初めて見た場合: 位置をデコードして追加し、後で同じ頂点を共有する角のためにコンパクトインデックスを記憶する。
								Uint32 compactIndex = static_cast<Uint32>(outPositions.size());
								remap[globalIndex] = compactIndex;
								outPositions.push_back(Vector3::Transform(DecodePosition(geometry_.compressedVertices_[globalIndex]), placement));
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
	void Crister::Reconstruct()
	{
		/// [EN] Nothing to do when full-precision vertices already exist, or when there is nothing to decode.
		/// [JP] フル精度の頂点が既にあるか、デコードするものが無ければ何もしない。
		if (!geometry_.vertices_.empty() || geometry_.compressedVertices_.empty())
		{
			return;
		}

		/// [EN] Decode every compressed vertex, with its skin attributes when present, back into vertices_ at the same global index.
		/// [JP] 全ての圧縮頂点を、スキン属性があればそれも含めて、同じグローバルインデックスのまま vertices_ へデコードし直す。
		geometry_.vertices_.reserve(geometry_.compressedVertices_.size());
		for (Size vertexIndex = 0; vertexIndex < geometry_.compressedVertices_.size(); vertexIndex++)
		{
			Vertex vertex = DecodeVertex(geometry_.compressedVertices_[vertexIndex]);
			if (vertexIndex < geometry_.compressedSkinVertices_.size())
			{
				DecodeSkin(geometry_.compressedSkinVertices_[vertexIndex], vertex.joints_, vertex.weights_);
			}
			geometry_.vertices_.push_back(vertex);
		}

		/// [EN] Rebuild a flat triangle list per SubMesh from its LOD 0 meshlets, resolving each corner to its global vertex index.
		/// [JP] SubMesh ごとに LOD 0 の meshlet からフラットな三角形リストを作り直し、各角をグローバル頂点インデックスへ解決する。
		DynamicArray<Uint32> flatIndices;
		for (SubMesh& subMesh : geometry_.subMeshes_)
		{
			Uint32 indexStart = static_cast<Uint32>(flatIndices.size());
			if (subMesh.clusterCount_ > 0 && subMesh.clusterOffset_ < geometry_.clusters_.size())
			{
				const Cluster& cluster = geometry_.clusters_[subMesh.clusterOffset_];
				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = geometry_.meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							flatIndices.push_back(geometry_.vertexIndices_[meshlet.vertexOffset_ + geometry_.primitiveIndices_[byteOffset + corner]]);
						}
					}
				}
			}

			/// [EN] Every SubMesh now indexes the whole vertex array directly, and morphs are dropped since the exported mesh has no LOD remap to keep them aligned.
			/// [JP] 各 SubMesh は頂点配列全体を直接指すようにする。書き出すメッシュには揃えを保つ LOD の対応が無いので、モーフは捨てる。
			subMesh.vertexOffset_ = 0;
			subMesh.vertexCount_ = static_cast<Uint32>(geometry_.vertices_.size());
			subMesh.indexOffset_ = indexStart;
			subMesh.indexCount_ = static_cast<Uint32>(flatIndices.size()) - indexStart;
			subMesh.morphs_.clear();
		}

		/// [EN] vertexIndices_ now holds the flat triangle list instead of meshlet-local indices.
		/// [JP] vertexIndices_ は meshlet ローカルなインデックスではなく、フラットな三角形リストを持つようになる。
		geometry_.vertexIndices_ = std::move(flatIndices);
	}

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
	Bool Crister::SoftbodyFinestVertices(DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outVertices.clear();
		outIndices.clear();

		if (geometry_.compressedVertices_.empty() || geometry_.subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Unlike BakeCollision, no remap: outVertices is a 1:1 copy of compressedVertices_ (decoded), so a global vertex index can be used directly as an index into it.
		/// [JP] BakeCollision と違いリマップは行わない: outVertices はcompressedVertices_ の 1:1 コピー（デコード済み）なので、グローバル頂点インデックスをそのままインデックスとして使える。
		outVertices.reserve(geometry_.compressedVertices_.size());
		for (const CompressedVertex& compressed : geometry_.compressedVertices_)
		{
			outVertices.push_back(DecodeVertex(compressed));
		}

		/// [EN] Tracks which vertices have already been moved to their node placement, so a vertex shared by several triangles is transformed once.
		/// [JP] どの頂点をノード配置へ移し終えたかを記録し、複数の三角形が共有する頂点を1回だけ変換する。
		DynamicArray<Bool> placedVertices(outVertices.size(), false);
		for (const SubMesh& subMesh : geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] LOD 0 is always the SubMesh's first cluster.
			/// [JP] LOD 0 は常に SubMesh の最初のクラスタ。
			Uint32 clusterIndex = subMesh.clusterOffset_;
			if (clusterIndex >= geometry_.clusters_.size())
			{
				continue;
			}

			/// [EN] A static SubMesh takes the transform of the first node that references its mesh; a skinned one stays in bind pose.
			/// [JP] 静的な SubMesh は、そのメッシュを参照する最初のノードの変換を使う。スキン付きのものはバインドポーズのまま。
			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(scene_.nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != scene_.nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			/// [EN] Normals go through the inverse transpose so non-uniform scale keeps them perpendicular, and a mirroring placement flips the tangent handedness.
			/// [JP] 法線は逆転置行列で変換し、非一様スケールでも面に垂直なままにする。鏡映する配置では接線の利き手を反転する。
			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = geometry_.clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = geometry_.meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = geometry_.vertexIndices_[meshlet.vertexOffset_ + geometry_.primitiveIndices_[byteOffset + corner]];

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
	Bool Crister::SoftbodyCoarsestVertices(DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outVertices.clear();
		outIndices.clear();

		if (geometry_.compressedVertices_.empty() || geometry_.subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Maps a QUANTISED bind-pose position to its slot in outVertices — unlike BakeCollision's remap (keyed by original global vertex index), this welds any two corners that land on the same position even if they came from different global indices (e.g. UV-seam-duplicated vertices). LOD simplification of the coarsest cluster can otherwise leave two distinct global indices at (numerically) the same position; feeding both into Jolt as separately connected vertices produces a zero-length edge, which SoftBodySharedSettings::CreateConstraints (its Dijkstra-based CalculateClosestKinematic) cannot handle. Quantised to a 1e-4 world-unit grid: fine enough that legitimately distinct vertices never collide, coarse enough to catch float round-trip noise between originally-identical positions.
		/// [JP] 量子化したバインドポーズ位置を outVertices 内のスロットへ対応付ける — BakeCollision のリマップ（元のグローバル頂点インデックスでキー）と異なり、これは元のグローバルインデックスが違っていても同じ位置に落ちる2つの角を溶接する（UV継ぎ目で複製された頂点など）。最粗クラスタの LOD簡略化は、（数値的に）同じ位置に異なる2つのグローバルインデックスを残すことがある。両方を別々に接続された頂点として Jolt に渡すと長さ0の辺ができ、SoftBodySharedSettings::CreateConstraints（Dijkstra ベースの CalculateClosestKinematic）はそれを扱えない。1e-4ワールド単位グリッドへ量子化: 正当に別々の頂点が衝突しない程度に細かく、元々同一だった位置間の浮動小数往復誤差を拾える程度に粗い。
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

		for (const SubMesh& subMesh : geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] Coarsest cluster, the same range as BakeCollision's Proxy detail.
			/// [JP] 最粗クラスタ。BakeCollision の Proxy 詳細度と同じ範囲。
			Uint32 clusterIndex = subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;
			if (clusterIndex >= geometry_.clusters_.size())
			{
				continue;
			}

			/// [EN] A static SubMesh takes the transform of the first node that references its mesh; a skinned one stays in bind pose.
			/// [JP] 静的な SubMesh は、そのメッシュを参照する最初のノードの変換を使う。スキン付きのものはバインドポーズのまま。
			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(scene_.nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != scene_.nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			/// [EN] Normals go through the inverse transpose so non-uniform scale keeps them perpendicular, and a mirroring placement flips the tangent handedness.
			/// [JP] 法線は逆転置行列で変換し、非一様スケールでも面に垂直なままにする。鏡映する配置では接線の利き手を反転する。
			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = geometry_.clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = geometry_.meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = geometry_.vertexIndices_[meshlet.vertexOffset_ + geometry_.primitiveIndices_[byteOffset + corner]];
						const CompressedVertex& compressed = geometry_.compressedVertices_[globalIndex];

						Vertex vertex;

						/// [EN] Only the position is decoded up front; it alone decides whether this corner welds onto an existing vertex.
						/// [JP] 先にデコードするのは位置だけ。既存の頂点へ溶接するかは位置だけで決まる。
						vertex.position_ = DecodePosition(compressed);
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

							vertex.normal_ = DecodeOctahedralNormal(compressed.normal_);
							vertex.tangent_ = DecodeTangent(compressed);
							vertex.texcoord_ = DecodeTexcoord(compressed);
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
	Vertex Crister::DecodeVertex(const CompressedVertex& compressed)const
	{
		/// [EN] Decode each field separately; the skin attributes live in a different array and are not part of this.
		/// [JP] 各フィールドを個別にデコードする。スキン属性は別の配列にあり、ここには含まない。
		Vertex vertex;
		vertex.position_ = DecodePosition(compressed);
		vertex.normal_ = DecodeOctahedralNormal(compressed.normal_);
		vertex.tangent_ = DecodeTangent(compressed);
		vertex.texcoord_ = DecodeTexcoord(compressed);
		return vertex;
	}

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
	Vector3 Crister::DecodeOctahedralNormal(Uint32 packed)const
	{
		/// [EN] Unpack x and y back to [-1,1] and rebuild z from the octahedron's |x| + |y| + |z| = 1.
		/// [JP] x と y を [-1,1] へ戻し、八面体の |x| + |y| + |z| = 1 から z を作り直す。
		Float ex = static_cast<Float>(packed & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
		Float ey = static_cast<Float>(packed >> 16) / 65535.0f * 2.0f - 1.0f;

		Vector3 n(ex, ey, 1.0f - std::abs(ex) - std::abs(ey));

		/// [EN] A negative z means the point was folded from the lower hemisphere; unfold it.
		/// [JP] z が負なら下半球から折り返した点なので、元に戻す。
		if (n.z < 0.0f)
		{
			Float nx = (1.0f - std::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
			Float ny = (1.0f - std::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
			n.x = nx;
			n.y = ny;
		}

		/// [EN] The octahedron point is not unit length; normalise it back onto the sphere.
		/// [JP] 八面体上の点は長さ 1 ではないので、正規化して球面へ戻す。
		n.Normalize();
		return n;
	}

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
	void Crister::Upload(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap)
	{
		gpu_.Upload(*this, device, cmdQueue, heap);
	}

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
	std::span<const Stage> Crister::Stages()const
	{
		return scene_.stages_; 
	}

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
	std::span<const Node> Crister::Nodes()const
	{
		return scene_.nodes_;
	}

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
	std::span<const PunctualLight> Crister::Lights()const
	{
		return scene_.lights_;
	}

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
	std::span<const SubMesh> Crister::SubMeshes()const
	{
		return geometry_.subMeshes_;
	}

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
	std::span<const Surface> Crister::Surfaces()const
	{
		return material_.surfaces_;
	}

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
	std::span<const Skin> Crister::Skins()const
	{
		return scene_.skins_;
	}

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
	std::span<const Cluster> Crister::Clusters()const
	{
		return geometry_.clusters_;
	}

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
	Int Crister::DefaultStage()const
	{
		return scene_.defaultStage_;
	}

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
	Int Crister::FindNodeIndex(const std::string& name)const
	{
		for (Size nodeIndex = 0; nodeIndex < scene_.nodes_.size(); nodeIndex++)
		{
			if (scene_.nodes_[nodeIndex].name_ == name)
			{
				return static_cast<Int>(nodeIndex);
			}
		}
		return -1;
	}

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
	Vector3 Crister::PositionMin()const
	{
		return geometry_.positionMin_;
	}

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
	Vector3 Crister::PositionExtent()const
	{
		return geometry_.positionExtent_;
	}

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
	Vector3 Crister::PlacedPositionMin()const
	{
		return gpu_.PlacedPositionMin();
	}

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
	Vector3 Crister::PlacedPositionExtent()const
	{
		return gpu_.PlacedPositionExtent();
	}

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
	Vector2 Crister::TexcoordMin()const
	{
		return geometry_.texcoordMin_;
	}

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
	Vector2 Crister::TexcoordExtent()const
	{
		return geometry_.texcoordExtent_;
	}

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
	D3D12_GPU_VIRTUAL_ADDRESS Crister::PositionBufferAddress()const
	{
		return gpu_.PositionBufferAddress();
	}

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
	Uint Crister::PositionBufferIndex()const
	{
		return gpu_.PositionBufferIndex();
	}

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
	Uint32 Crister::VertexCount()const
	{
		return gpu_.VertexCount();
	}

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
	D3D12_GPU_VIRTUAL_ADDRESS Crister::IndexBufferAddress()const
	{
		return gpu_.IndexBufferAddress();
	}

	/**
	* [EN]
	* Triangle count of the flat index buffer IndexBufferAddress() points to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* IndexBufferAddress() が指すフラットインデックスバッファの三角形数。
	*/
	Uint32 Crister::IndexCount()const
	{
		return gpu_.IndexCount();
	}

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
	Uint Crister::TextureBindlessIndex(Uint32 textureIndex)const
	{
		return gpu_.TextureBindlessIndex(textureIndex);
	}

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
	Bool Crister::MaterialsDirty()const
	{
		return gpu_.MaterialsDirty();
	}

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
	void Crister::ClearMaterialsDirty()const
	{
		gpu_.ClearMaterialsDirty();
	}

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
	Uint Crister::VertexBufferIndex()const
	{
		return gpu_.VertexBufferIndex();
	}

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
	Uint Crister::IndexBufferIndex()const
	{
		return gpu_.IndexBufferIndex();
	}

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
	Uint Crister::SkinVertexBufferIndex()const
	{
		return gpu_.SkinVertexBufferIndex();
	}

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
	Uint Crister::MorphDeltaBufferIndex()const
	{
		return gpu_.MorphDeltaBufferIndex();
	}

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
	Uint Crister::VertexMorphSourceBufferIndex()const
	{
		return gpu_.VertexMorphSourceBufferIndex();
	}

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
	Bool Crister::ProxySkinned()const
	{
		return gpu_.ProxySkinned();
	}

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
	Uint Crister::ProxySkinVertexBufferIndex()const
	{
		return gpu_.ProxySkinVertexBufferIndex();
	}

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
	Uint Crister::ProxyMorphDeltaBufferIndex()const
	{
		return gpu_.ProxyMorphDeltaBufferIndex();
	}

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
	std::span<const Matrix> Crister::SubMeshPlacement(Size subMeshIndex)const
	{
		return gpu_.SubMeshPlacement(subMeshIndex);
	}

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
	void Crister::CopyMorph(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination)const
	{
		gpu_.CopyMorph(cmdList, destination);
	}

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
	void Crister::MakeClusterResident(Uint32 clusterIndex)
	{
		gpu_.MakeClusterResident(*this, clusterIndex);
	}

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
	void Crister::MakeTextureMipResident(Uint32 textureIndex, Uint32 targetMip)
	{
		gpu_.MakeTextureMipResident(*this, textureIndex, targetMip);
	}

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
	void Crister::MakePoolResident()
	{
		gpu_.MakePoolResident(*this);
	}

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
	void Crister::EvictCluster(Uint32 clusterIndex)
	{
		gpu_.EvictCluster(clusterIndex);
	}

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
	void Crister::EvictTextureMip(Uint32 textureIndex)
	{
		gpu_.EvictTextureMip(textureIndex);
	}

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
	void Crister::EvictPool()
	{
		gpu_.EvictPool();
	}

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
	void Crister::TouchCluster(Uint32 clusterIndex, Uint64 frame)
	{
		gpu_.TouchCluster(clusterIndex, frame);
	}

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
	void Crister::TouchTexture(Uint32 textureIndex, Uint64 frame)
	{
		gpu_.TouchTexture(textureIndex, frame);
	}

	/**
	* [EN]
	* Whether the cluster's page is currently GPU-resident.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタのページが現在 GPU に常駐しているか。
	*/
	Bool Crister::ClusterResident(Uint32 clusterIndex)const
	{
		return gpu_.ClusterResident(clusterIndex);
	}

	/**
	* [EN]
	* Whether the texture has streamed all the way in to mip 0 (its finest).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* テクスチャがミップ 0（最も細かい）まで完全にストリームインしているか。
	*/
	Bool Crister::TextureResident(Uint32 textureIndex)const
	{
		return gpu_.TextureResident(textureIndex);
	}

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
	void Crister::EvictClusterBudget(Uint64 currentFrame)
	{
		CristerGpu::EvictClusterBudget(currentFrame);
	}

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
	void Crister::EvictTextureBudget(Uint64 currentFrame)
	{
		CristerGpu::EvictTextureBudget(currentFrame);
	}

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
	Uint Crister::ClusterVertexBufferIndex(Uint32 clusterIndex)const
	{
		return gpu_.ClusterVertexBufferIndex(clusterIndex);
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's meshlet buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの meshlet バッファの bindless SRV。
	*/
	Uint Crister::ClusterMeshletBufferIndex(Uint32 clusterIndex)const
	{
		return gpu_.ClusterMeshletBufferIndex(clusterIndex);
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's meshlet bound buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの meshlet バウンドバッファの bindless SRV。
	*/
	Uint Crister::ClusterMeshletBoundBufferIndex(Uint32 clusterIndex)const
	{
		return gpu_.ClusterMeshletBoundBufferIndex(clusterIndex);
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's vertex indices buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの頂点インデックスバッファの bindless SRV。
	*/
	Uint Crister::ClusterVertexIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return gpu_.ClusterVertexIndicesBufferIndex(clusterIndex);
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's primitive indices buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページのプリミティブインデックスバッファの bindless SRV。
	*/
	Uint Crister::ClusterPrimitiveIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return gpu_.ClusterPrimitiveIndicesBufferIndex(clusterIndex);
	}

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
	Bool Crister::StandaloneVertices(Uint32 clusterIndex)const
	{
		return gpu_.StandaloneVertices(clusterIndex);
	}

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
	Uint32 Crister::TextureFinestMip(Uint32 textureIndex)const
	{
		return gpu_.TextureFinestMip(textureIndex);
	}

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
	Uint32 Crister::TextureDesiredMip(Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const
	{
		return gpu_.TextureDesiredMip(*this, textureIndex, texcoordDensity, worldScale, pixelsPerUnit);
	}

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
	void Crister::CumulateTransforms()
	{
		/// [EN] Without a valid default stage there is no root to start from.
		/// [JP] 正しいデフォルトステージが無ければ、始めるルートが無い。
		if (scene_.defaultStage_ < 0 || static_cast<Size>(scene_.defaultStage_) >= scene_.stages_.size())
		{
			return;
		}

		/// [EN] The top of the stack is the global transform of the parent of the node being visited.
		/// [JP] スタックの一番上が、訪れているノードの親のグローバル変換。
		std::stack<Matrix> parentGlobalTransforms;
		std::function<void(Int)> traverse = [&](Int nodeIndex)
			{
				if (nodeIndex < 0 || static_cast<Size>(nodeIndex) >= scene_.nodes_.size())
				{
					return;
				}

				Node& node = scene_.nodes_[nodeIndex];
				Matrix scale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix rotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix translation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				/// [EN] Local S/R/T in row-vector order, then onto the parent's global transform.
				/// [JP] ローカルの S/R/T を行ベクトルの順で掛け、親のグローバル変換に重ねる。
				Matrix localTransform = scale * rotation * translation;
				node.globalTransform_ = localTransform * parentGlobalTransforms.top();

				/// [EN] Each child sees this node's global transform as its parent's.
				/// [JP] 各子は、このノードのグローバル変換を親のものとして受け取る。
				for (Int childIndex : node.children_)
				{
					parentGlobalTransforms.push(node.globalTransform_);
					traverse(childIndex);
					parentGlobalTransforms.pop();
				}
			};

		/// [EN] Every root starts under the identity.
		/// [JP] 各ルートは単位行列の下から始める。
		for (Int nodeIndex : scene_.stages_[scene_.defaultStage_].nodes_)
		{
			parentGlobalTransforms.push(Matrix::Identity);
			traverse(nodeIndex);
			parentGlobalTransforms.pop();
		}
	}

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
	Vector3 Crister::DecodePosition(const CompressedVertex& compressed)const
	{
		/// [EN] Each 16-bit UNORM maps back across the AABB extent from its minimum corner.
		/// [JP] 各 16bit UNORM を、AABB の最小コーナーから大きさの範囲へ戻す。
		return geometry_.positionMin_ + Vector3(
			static_cast<Float>(compressed.positionXY_ & 0xFFFF) / 65535.0f * geometry_.positionExtent_.x,
			static_cast<Float>(compressed.positionXY_ >> 16) / 65535.0f * geometry_.positionExtent_.y,
			static_cast<Float>(compressed.positionZTexU_ & 0xFFFF) / 65535.0f * geometry_.positionExtent_.z);
	}

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
	Vector2 Crister::DecodeTexcoord(const CompressedVertex& compressed)const
	{
		/// [EN] u sits in the upper half of positionZTexU_ and v in the lower half of texVTangent_.
		/// [JP] u は positionZTexU_ の上位16bit、v は texVTangent_ の下位16bit にある。
		return geometry_.texcoordMin_ + Vector2(
			static_cast<Float>(compressed.positionZTexU_ >> 16) / 65535.0f * geometry_.texcoordExtent_.x,
			static_cast<Float>(compressed.texVTangent_ & 0xFFFF) / 65535.0f * geometry_.texcoordExtent_.y);
	}

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
	Vector4 Crister::DecodeTangent(const CompressedVertex& compressed)const
	{
		/// [EN] The tangent is the upper 16 bits: octahedral x in 8 bits, y in 7 bits and the handedness sign on top.
		/// [JP] 接線は上位16bit。octahedral の x が 8bit、y が 7bit、最上位が利き手の符号。
		Uint32 tangentBits = compressed.texVTangent_ >> 16;

		/// [EN] Widen both components back to 16 bits so the normal decoder can be reused.
		/// [JP] 法線のデコードを使い回せるよう、両成分を 16bit へ広げ直す。
		Uint32 octX16 = static_cast<Uint32>(static_cast<Float>(tangentBits & 0xFF) / 255.0f * 65535.0f + 0.5f);
		Uint32 octY16 = static_cast<Uint32>(static_cast<Float>((tangentBits >> 8) & 0x7F) / 127.0f * 65535.0f + 0.5f);
		Float sign = (tangentBits >> 15) ? 1.0f : -1.0f;

		Vector3 xyz = DecodeOctahedralNormal(octX16 | (octY16 << 16));
		return Vector4(xyz.x, xyz.y, xyz.z, sign);
	}

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
	void Crister::DecodeSkin(const CompressedSkinVertex& compressed, XmUint4& outJoints, Vector4& outWeights)const
	{
		/// [EN] Unpack the four 16-bit joint indices.
		/// [JP] 16bit のジョイントインデックスを4つ取り出す。
		outJoints.x = compressed.jointsXY_ & 0xFFFF;
		outJoints.y = compressed.jointsXY_ >> 16;
		outJoints.z = compressed.jointsZW_ & 0xFFFF;
		outJoints.w = compressed.jointsZW_ >> 16;

		outWeights = Vector4(static_cast<Float>(compressed.weights_ & 0xFF), static_cast<Float>((compressed.weights_ >> 8) & 0xFF), static_cast<Float>((compressed.weights_ >> 16) & 0xFF), static_cast<Float>(compressed.weights_ >> 24)) / 255.0f;

		/// [EN] Dividing by the sum both undoes the 8-bit scale and makes the weights add up to 1.
		/// [JP] 合計で割ることで、8bit の倍率を戻すと同時に重みの合計を 1 にする。
		Float weightSum = outWeights.x + outWeights.y + outWeights.z + outWeights.w;
		outWeights /= Max(weightSum, 1e-6f);
	}

}
