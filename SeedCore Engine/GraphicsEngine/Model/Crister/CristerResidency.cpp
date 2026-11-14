#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerFlattener.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	/**
	* [EN]
	* Leaves the shared streaming registry, so budget eviction never walks
	* into this Crister while it is being torn down. The GPU resources and
	* descriptors are returned by Release(), which the owner calls before
	* destroying the Crister.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のストリーミングレジストリから外れ、破棄中のこの Crister を予算に
	* よる追い出しが辿らないようにする。GPU リソースとディスクリプタは、
	* 持ち主が Crister を破棄する前に呼ぶ Release() が返す。
	*/
	CristerResidency::~CristerResidency()
	{
		auto found = std::ranges::find(streamingRegistry_, this);
		if (found != streamingRegistry_.end())
		{
			streamingRegistry_.erase(found);
		}
	}

	/**
	* [EN]
	* Releases every resident streaming page (pinned included — the model
	* itself is going away), the RT proxy and the morph buffers, and
	* returns their descriptor indices to heap. Every GPU resource is
	* handed to the deferred-reclaim ring rather than dying with this
	* object, since frames still in flight may be drawing from these exact
	* buffers and textures. No-op for a Crister that never ran Upload().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 常駐中のストリーミングページ（モデル自体が消えるためピン留め込み）、
	* RT プロキシ、モーフのバッファをすべて解放し、ディスクリプタ
	* インデックスを heap へ返す。GPU リソースはこのオブジェクトと一緒に
	* 死なせず、遅延回収リングへ渡す — インフライトのフレームがまさに
	* これらのバッファやテクスチャで描画している可能性があるため。
	* Upload() を通っていない Crister では何もしない。
	*/
	void CristerResidency::Release(BindlessHeap* heap)
	{
		/// [EN] A Crister that never ran Upload() holds no GPU resources and no descriptors.
		/// [JP] Upload() を通っていない Crister は GPU リソースもディスクリプタも持たない。
		if (!uploaded_)
		{
			return;
		}

		/// [EN] Leave the registry first, so budget eviction stops seeing pages that are about to go.
		/// [JP] 先にレジストリから外れ、これから消えるページを予算による追い出しが見ないようにする。
		auto found = std::ranges::find(streamingRegistry_, this);
		if (found != streamingRegistry_.end())
		{
			streamingRegistry_.erase(found);
		}

		/// [EN] Release every resident page (pinned included — the model itself is going away) and give the descriptor indices back to the heap. Each resource goes to the deferred-reclaim ring because the frames still in flight are drawing from exactly these buffers and textures.
		/// [JP] 常駐ページをすべて解放し（モデル自体が消えるためピン留め込み）、ディスクリプタインデックスをヒープへ返す。インフライトのフレームはまさにこれらのバッファとテクスチャで描画している最中なので、各リソースは遅延回収リングへ渡す。
		for (StreamingGeometry& page : streamingGeometry_)
		{
			if (!page.resident_)
			{
				continue;
			}
			heap->Release(std::move(page.meshletResource_), page.meshletBufferIndex_);
			heap->Release(std::move(page.meshletBoundResource_), page.meshletBoundBufferIndex_);
			heap->Release(std::move(page.vertexIndicesResource_), page.vertexIndicesBufferIndex_);
			heap->Release(std::move(page.primitiveIndicesResource_), page.primitiveIndicesBufferIndex_);

			/// [EN] A page that reads the shared pool has no vertex buffer of its own; the pool is released once below.
			/// [JP] 共有プールを読むページは自前の頂点バッファを持たない。プールは下で1度だけ解放する。
			if (page.ownsVertices_)
			{
				heap->Release(std::move(page.vertexResource_), page.vertexBufferIndex_);
			}

			/// [EN] Take the page out of the geometry total so the budget stays accurate for the remaining Crister.
			/// [JP] 残りの Crister にとって予算が正しいままになるよう、ページをジオメトリの合計から外す。
			totalResidentGeometryBytes_ -= page.sizeBytes_;
			page.resident_ = false;
			++version_;
		}

		/// [EN] Release the shared LOD 0 vertex pool, pinned or not.
		/// [JP] ピン留めかどうかに関わらず、共有 LOD 0 頂点プールを解放する。
		if (poolResident_)
		{
			heap->Release(std::move(poolResource_), poolBufferIndex_);
			totalResidentGeometryBytes_ -= poolSizeBytes_;
			poolResident_ = false;
			++version_;
		}

		/// [EN] Release both resident mips of every texture, the pinned fallback included.
		/// [JP] 全テクスチャの常駐中の2つのミップを、ピン留めのフォールバックも含めて解放する。
		for (StreamingTexture& streamingTexture : streamingTextures_)
		{
			if (streamingTexture.pinnedMip_.resource_)
			{
				totalResidentTextureBytes_ -= streamingTexture.pinnedMip_.sizeBytes_;
				heap->Release(std::move(streamingTexture.pinnedMip_.resource_), streamingTexture.pinnedMip_.bindlessIndex_);
			}
			if (streamingTexture.currentMip_.resource_)
			{
				totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
				heap->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
			}
		}

		/// [EN] The RT proxy geometry (vertex/position/index/skin), its morph pools, and the raster-side morph buffers outlive this object the same way the streaming pages above do: a reflection/GI TLAS instance built over this Crister can linger for a few frames after the model is unloaded (see RaytracingRenderer's pendingBlasEviction_ deferral), and its closest-hit shader fetches vertex attributes from exactly these buffers. Hand them to the deferred-reclaim ring and return their bindless SRV slots instead of letting the ComPtr members free them synchronously here.
		/// [JP] RT プロキシジオメトリ（頂点/位置/インデックス/スキン）、そのモーフプール、およびラスタ側のモーフバッファは、上の streamingページと同じくこのオブジェクトより長生きする: この Crister の上に構築された反射/GI の TLAS インスタンスは、モデルのアンロード後も数フレーム残り得る（RaytracingRenderer のpendingBlasEviction_ 遅延参照）。そのヒットシェーダはまさにこれらのバッファから頂点属性を読む。ComPtr メンバにここで同期的に解放させず、遅延回収リングへ渡し、bindless SRVスロットを返す。
		heap->Release(std::move(vertexResource_), vertexBufferIndex_);
		heap->Release(std::move(skinVertexResource_), skinVertexBufferIndex_);
		heap->Release(std::move(indexResource_), indexBufferIndex_);
		heap->Release(std::move(positionResource_), positionBufferIndex_);
		heap->Release(std::move(raytracingSkinVertexResource_), raytracingSkinVertexBufferIndex_);
		heap->Release(std::move(raytracingMorphDeltaResource_), raytracingMorphDeltaBufferIndex_);
		heap->Release(std::move(morphDeltaResource_), morphDeltaBufferIndex_);
		heap->Release(std::move(vertexMorphSourceResource_), vertexMorphSourceBufferIndex_);

		/// [EN] Everything is returned; a second Release() has nothing left to do.
		/// [JP] すべて返したので、2回目の Release() には何も残っていない。
		uploaded_ = false;
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
	* is drawable at its fallback LOD/resolution as soon as lane's work
	* completes. Finer pages/mips stream in later on demand
	* (MakeClusterResident/MakeTextureMipResident, driven by
	* ModelRenderer::Gather). Every copy is only recorded on lane; the
	* caller submits it and waits for its fence before the model is drawn.
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
	* ジオメトリページ/テクスチャミップを常駐させ、lane の処理が終われば
	* フォールバック LOD/解像度で描画できるようにする。より細かい
	* ページ/ミップは後でオンデマンドにストリームインする
	* （MakeClusterResident/MakeTextureMipResident、ModelRenderer::Gather
	* から駆動）。コピーは lane に記録するだけで、投入とフェンスの待ちは
	* モデルを描画する前に呼び出し側が行う。
	*/
	void CristerResidency::Upload(Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap)
	{
		HRESULT hr{ S_OK };

		/// [EN] From here on this Crister owns GPU resources that Release() must return.
		/// [JP] ここから先、この Crister は Release() で返すべき GPU リソースを持つ。
		uploaded_ = true;

		/// [EN] Resolve each SubMesh's placements: the global transform of every node that references its mesh, or a single identity for a skinned or unreferenced SubMesh.
		/// [JP] 各 SubMesh の配置を解決する。そのメッシュを参照する全ノードのグローバル変換で、スキン付きか参照されない SubMesh は単位行列1つ。
		subMeshPlacement_.assign(crister.geometry_.subMeshes_.size(), DynamicArray<Matrix>{});
		for (Size subMeshIndex = 0; subMeshIndex < crister.geometry_.subMeshes_.size(); subMeshIndex++)
		{
			const SubMesh& subMesh = crister.geometry_.subMeshes_[subMeshIndex];
			DynamicArray<Matrix>& placements = subMeshPlacement_[subMeshIndex];

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
		}

		/// [EN] Derive each cluster's page ranges from its meshlet slice. buildMeshletsFromIndices appends sequentially, so a cluster's meshlets occupy contiguous slices of vertexIndices_ and primitiveIndices_; LOD>=1 clusters also own a contiguous vertex range (QEM appends fresh vertex copies per LOD). LOD 0 clusters reference the original vertex pool instead.
		/// [JP] 各クラスタのページレンジを meshlet 列から導出する。buildMeshletsFromIndices は逐次追加なので、クラスタの meshlet はvertexIndices_ / primitiveIndices_ の連続スライスを占有する。LOD>=1 クラスタは専用の連続頂点範囲も所有する（QEM が LOD ごとに新頂点を追加）。LOD 0 クラスタは元の頂点プールを参照する。
		streamingGeometry_.assign(crister.geometry_.clusters_.size(), StreamingGeometry{});
		poolVertexEnd_ = 0;
		for (Size clusterIndex = 0; clusterIndex < crister.geometry_.clusters_.size(); clusterIndex++)
		{
			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];
			StreamingGeometry& page = streamingGeometry_[clusterIndex];
			if (cluster.meshletCount_ == 0)
			{
				continue;
			}

			/// [EN] The page's index and primitive slices run from the cluster's first meshlet to the end of its last one.
			/// [JP] ページのインデックスとプリミティブのスライスは、クラスタの最初の meshlet から最後の meshlet の終わりまで。
			const MeshletDesc& first = crister.geometry_.meshlets_[cluster.meshletOffset_];
			const MeshletDesc& last = crister.geometry_.meshlets_[cluster.meshletOffset_ + cluster.meshletCount_ - 1];
			page.vertexIndexBegin_ = first.vertexOffset_;
			page.vertexIndexEnd_ = last.vertexOffset_ + last.vertexCount_;
			page.primitiveBegin_ = first.triangleOffset_;
			page.primitiveEnd_ = last.triangleOffset_ + last.triangleCount_ * 3;
			page.ownsVertices_ = cluster.lodLevel_ >= 1;

			/// [EN] Find the range of global vertices the page's indices touch.
			/// [JP] ページのインデックスが触れるグローバル頂点の範囲を求める。
			Uint32 minVertex = 0xFFFFFFFF;
			Uint32 maxVertex = 0;
			for (Uint32 index = page.vertexIndexBegin_; index < page.vertexIndexEnd_; index++)
			{
				minVertex = Min(minVertex, crister.geometry_.vertexIndices_[index]);
				maxVertex = Max(maxVertex, crister.geometry_.vertexIndices_[index]);
			}

			/// [EN] An LOD >= 1 page owns exactly that range; an LOD 0 page reads the pool, which grows to cover every vertex an LOD 0 page touches.
			/// [JP] LOD >= 1 のページはちょうどその範囲を持つ。LOD 0 のページはプールを読み、プールは LOD 0 のページが触れる全ての頂点を含むまで広がる。
			if (page.ownsVertices_)
			{
				page.vertexBegin_ = minVertex;
				page.vertexEnd_ = maxVertex + 1;
			}
			else
			{
				page.vertexBegin_ = 0;
				page.vertexEnd_ = maxVertex + 1;
				poolVertexEnd_ = Max(poolVertexEnd_, maxVertex + 1);
			}
		}

		/// [EN] Pin the pages that must never leave VRAM: the coarsest cluster of every SubMesh (guaranteed fallback LOD) and, for skinned SubMeshes, LOD 0 (their only drawable LOD). The vertex pool is pinned when any pinned LOD 0 cluster references it.
		/// [JP] VRAM から出してはいけないページをピン留めする: 各 SubMesh の最粗クラスタ（フォールバック LOD の保証）と、スキンド SubMesh のLOD 0（唯一描画可能な LOD）。ピン留めされた LOD 0 クラスタがあれば頂点プールもピン留めする。
		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}
			streamingGeometry_[subMesh.clusterOffset_ + subMesh.clusterCount_ - 1].pinned_ = true;
			if (subMesh.skinIndex_ >= 0)
			{
				streamingGeometry_[subMesh.clusterOffset_].pinned_ = true;
			}
		}

		/// [EN] The pool is pinned as soon as any pinned page reads it.
		/// [JP] ピン留めされたページが1つでもプールを読むなら、プールもピン留めする。
		for (Size clusterIndex = 0; clusterIndex < streamingGeometry_.size(); clusterIndex++)
		{
			if (streamingGeometry_[clusterIndex].pinned_ && !streamingGeometry_[clusterIndex].ownsVertices_)
			{
				poolPinned_ = true;
			}
		}

		/// [EN] Skinning attributes for the pool range (skinned SubMeshes are LOD 0 only, so pool-sized coverage is sufficient). Always resident when skins exist — skinned LOD 0 is pinned anyway.
		/// [JP] プール範囲分のスキニング属性（スキンド SubMesh は LOD 0 のみなのでプールサイズで足りる）。スキンがあれば常に常駐 — どのみちスキンド LOD 0 はピン留めされている。
		if (!crister.scene_.skins_.empty() && poolVertexEnd_ > 0)
		{
			hr = CreateStaticBufferUnbounded(device, lane, crister.geometry_.compressedSkinVertices_.data(), poolVertexEnd_, sizeof(CompressedSkinVertex), skinVertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "スキンバーテックスバッファの生成に失敗しました");
			skinVertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, skinVertexResource_.Get(), poolVertexEnd_, sizeof(CompressedSkinVertex));
		}

		CristerFlattener flattener;
		flattener.Flatten(crister, subMeshPlacement_);
		placedPositionMin_ = flattener.PlacedPositionMin();
		placedPositionExtent_ = flattener.PlacedPositionExtent();

		/// [EN] Create the RT proxy buffers and their SRVs: compressed vertices, float3 positions, triangle indices, and skin and morph data when present.
		/// [JP] RT プロキシのバッファと SRV を作る。圧縮頂点、float3 の位置、三角形インデックス、あればスキンとモーフのデータ。
		if (!flattener.TriangleIndices().empty())
		{
			hr = CreateStaticBufferUnbounded(device, lane, flattener.Vertices().data(), flattener.Vertices().size(), sizeof(CompressedVertex), vertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用バーテックスバッファの生成に失敗しました");
			vertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, vertexResource_.Get(), static_cast<Uint>(flattener.Vertices().size()), sizeof(CompressedVertex));
			proxyVertexCount_ = static_cast<Uint32>(flattener.Vertices().size());

			hr = CreateStaticBufferUnbounded(device, lane, flattener.Positions().data(), flattener.Positions().size(), sizeof(Vector3), positionResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用ポジションバッファの生成に失敗しました");
			positionBufferIndex_ = CreateStructuredShaderResourceView(device, heap, positionResource_.Get(), static_cast<Uint>(flattener.Positions().size()), sizeof(Vector3));

			hr = CreateStaticBufferUnbounded(device, lane, flattener.TriangleIndices().data(), flattener.TriangleIndices().size(), sizeof(Uint32), indexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用インデックスバッファの生成に失敗しました");
			triangleIndexCount_ = static_cast<Uint32>(flattener.TriangleIndices().size());
			indexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, indexResource_.Get(), triangleIndexCount_, sizeof(Uint32));

			/// [EN] The proxy skin buffer is created only when it lines up one to one with the proxy vertices.
			/// [JP] プロキシのスキンバッファは、プロキシ頂点と 1 対 1 で揃っているときだけ作る。
			if (flattener.SkinVerticesBuilt() && flattener.SkinVertices().size() == flattener.Vertices().size())
			{
				hr = CreateStaticBufferUnbounded(device, lane, flattener.SkinVertices().data(), flattener.SkinVertices().size(), sizeof(CompressedSkinVertex), raytracingSkinVertexResource_.ReleaseAndGetAddressOf());
				SC_HR_CHECK(hr, "レイトレーシング用スキンバーテックスバッファの生成に失敗しました");
				raytracingSkinVertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, raytracingSkinVertexResource_.Get(), static_cast<Uint>(flattener.SkinVertices().size()), sizeof(CompressedSkinVertex));
			}

			if (!flattener.MorphDeltas().empty())
			{
				hr = CreateStaticBufferUnbounded(device, lane, flattener.MorphDeltas().data(), flattener.MorphDeltas().size(), sizeof(Vector3), raytracingMorphDeltaResource_.ReleaseAndGetAddressOf());
				SC_HR_CHECK(hr, "レイトレーシング用モーフデルタバッファの生成に失敗しました");
				raytracingMorphDeltaBufferIndex_ = CreateStructuredShaderResourceView(device, heap, raytracingMorphDeltaResource_.Get(), static_cast<Uint>(flattener.MorphDeltas().size()), sizeof(Vector3));
			}
		}

		/// [EN] Raster morph support: unlike the RT proxy's morph delta pool above, this is baked straight from Morph::positionDeltas_ (no compaction/remap — every SubMesh's original vertexCount_ deltas per target, back to back) since the raster path already has its own per-vertex remap (vertexMorphSource_) to resolve any streamed LOD's vertex back to this buffer's indexing. See the raster morph blend in the model mesh shaders and Model/Material/MaterialResolveCS.hlsl/Model/Transparent/TransparentMS.hlsl.
		/// [JP] ラスタのモーフ対応: 上の RT プロキシ用モーフデルタプールと違い、Morph::positionDeltas_ からそのまま焼き込む(圧縮/リマップ無し — 各 SubMesh のオリジナル vertexCount_ 個のデルタをターゲットごとに連続で)。ラスタ経路はストリームされたどの LOD の頂点もこのバッファの番号付けへ逆引きする独自の頂点リマップ(vertexMorphSource_)を既に持つため。モデル用メッシュシェーダーと Model/Material/MaterialResolveCS.hlsl/Model/Transparent/TransparentMS.hlsl のラスタのモーフブレンド参照。
		DynamicArray<Vector3> morphDeltas;
		for (SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.morphs_.empty())
			{
				continue;
			}

			/// [EN] Each morphing SubMesh appends its targets' deltas back to back and records where they start.
			/// [JP] モーフする各 SubMesh は、ターゲットのデルタを続けて追加し、その先頭を記録する。
			subMesh.morphDeltaOffset_ = static_cast<Uint32>(morphDeltas.size());
			for (const Morph& morph : subMesh.morphs_)
			{
				morphDeltas.insert(morphDeltas.end(), morph.positionDeltas_.begin(), morph.positionDeltas_.end());
			}
		}

		/// [EN] Create the raster delta pool together with the vertex-to-original map the shaders resolve LOD vertices through.
		/// [JP] ラスタ用のデルタプールを、シェーダーが LOD の頂点を解決するのに使う元頂点への対応と一緒に作る。
		if (!morphDeltas.empty())
		{
			hr = CreateStaticBufferUnbounded(device, lane, morphDeltas.data(), morphDeltas.size(), sizeof(Vector3), morphDeltaResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "ラスタ用モーフデルタバッファの生成に失敗しました");
			morphDeltaBufferIndex_ = CreateStructuredShaderResourceView(device, heap, morphDeltaResource_.Get(), static_cast<Uint>(morphDeltas.size()), sizeof(Vector3));

			hr = CreateStaticBufferUnbounded(device, lane, crister.geometry_.vertexMorphSource_.data(), crister.geometry_.vertexMorphSource_.size(), sizeof(Uint32), vertexMorphSourceResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "頂点モーフソースバッファの生成に失敗しました");
			vertexMorphSourceBufferIndex_ = CreateStructuredShaderResourceView(device, heap, vertexMorphSourceResource_.Get(), static_cast<Uint>(crister.geometry_.vertexMorphSource_.size()), sizeof(Uint32));
		}

		/// [EN] Set up per-texture streaming state (see StreamingTexture) from the BC7-compressed bitmaps baked by BakeBitmap(). No GPU work here — MakeTextureMipResident() below uploads mips on demand, starting with each texture's pinned coarsest mip.
		/// [JP] BakeBitmap() が焼いた BC7 圧縮ビットマップから、テクスチャごとのストリーミング状態を準備する（StreamingTexture 参照）。ここではGPU 作業を行わない — 下の MakeTextureMipResident() が各テクスチャのピン留め最粗ミップから始めてオンデマンドでアップロードする。
		streamingTextures_.assign(crister.material_.bitmaps_.size(), StreamingTexture{});
		for (Size textureIndex = 0; textureIndex < crister.material_.bitmaps_.size(); textureIndex++)
		{
			const Bitmap& bitmap = crister.material_.bitmaps_[textureIndex];
			if (bitmap.width_ <= 0 || bitmap.height_ <= 0 || bitmap.cacheData_.empty() || bitmap.mipCount_ <= 0)
			{
				continue;
			}

			/// [EN] Only mips that are legal as a STANDALONE single-mip BC7 resource can be streamed: D3D12 requires a block-compressed resource with MipLevels == 1 to have both dimensions a non-zero multiple of 4 (sub-4 sizes such as the 2x2/1x1 tail BakeBitmap generates are only legal as inner levels of a longer chain). Streaming the whole chain would therefore fail to create the coarsest levels and leave the material with no SRV at all. Clamp mipCount_ to the leading run of legal mips, so the pinned fallback is 4x4 rather than a flat 1x1 — and never sub-4.
			/// [JP] 「単一ミップの独立した BC7 リソース」として合法なミップだけをストリーミング対象にする: D3D12 はブロック圧縮リソースのMipLevels == 1 のとき、幅・高さの両方が 0 でない 4 の倍数であることを要求する(BakeBitmap が生成する末尾の 2x2/1x1 のような4 未満の寸法は、より長いチェーンの内側の段としてのみ合法)。チェーン全体をストリーミング対象にすると最粗段の生成に失敗し、マテリアルに SRV が 1 つも無い状態になってしまう。合法なミップが続く範囲で mipCount_ を打ち切り、ピン留めフォールバックが単色の 1x1 ではなく 4x4 になるようにする(4 未満は作らない)。
			Uint32 streamableMipCount = 0;
			Int mipWidth = bitmap.width_;
			Int mipHeight = bitmap.height_;
			while (streamableMipCount < static_cast<Uint32>(bitmap.mipCount_) && mipWidth >= 4 && mipHeight >= 4 && mipWidth % 4 == 0 && mipHeight % 4 == 0)
			{
				streamableMipCount++;
				mipWidth /= 2;
				mipHeight /= 2;
			}
			if (streamableMipCount == 0)
			{
				SC_LOG_WARNING("テクスチャ {} の寸法({}x{})が BC7 の単一ミップリソースとして不正なため、ストリーミング対象外にします。", textureIndex, bitmap.width_, bitmap.height_);
				continue;
			}

			StreamingTexture& streamingTexture = streamingTextures_[textureIndex];
			streamingTexture.mipCount_ = streamableMipCount;

			/// [EN] topResidentMip_ = mipCount_ means nothing is resident yet; the pinned mip is uploaded below.
			/// [JP] topResidentMip_ = mipCount_ はまだ何も常駐していないことを表す。ピン留めのミップは下でアップロードする。
			streamingTexture.topResidentMip_ = streamingTexture.mipCount_;
			streamingTexture.valid_ = true;
		}

		/// [EN] Register with the streaming bookkeeping, then bring the pinned geometry pages and texture mips in so the model is immediately drawable at its fallback LOD/resolution. Finer pages/mips stream in on demand from ModelRenderer::Gather.
		/// [JP] ストリーミング管理へ登録し、ピン留めジオメトリページとテクスチャミップをアップロードしてフォールバック LOD/解像度で即描画可能にする。より細かいページ/ミップは ModelRenderer::Gather からオンデマンドで常駐する。
		if (!std::ranges::contains(streamingRegistry_, this))
		{
			streamingRegistry_.push_back(this);
		}
		for (Size clusterIndex = 0; clusterIndex < streamingGeometry_.size(); clusterIndex++)
		{
			if (streamingGeometry_[clusterIndex].pinned_)
			{
				MakeClusterResident(crister, device, lane, heap, static_cast<Uint32>(clusterIndex));
			}
		}

		/// [EN] Upload each streamable texture's coarsest mip, which stays pinned.
		/// [JP] ストリーミングする各テクスチャの最も粗いミップをアップロードする。これはピン留めのまま残る。
		for (Size textureIndex = 0; textureIndex < streamingTextures_.size(); textureIndex++)
		{
			if (streamingTextures_[textureIndex].valid_)
			{
				MakeTextureMipResident(crister, device, lane, heap, static_cast<Uint32>(textureIndex), streamingTextures_[textureIndex].mipCount_ - 1);
			}
		}
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
	Vector3 CristerResidency::PlacedPositionMin()const
	{
		return placedPositionMin_;
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
	Vector3 CristerResidency::PlacedPositionExtent()const
	{
		return placedPositionExtent_;
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
	D3D12_GPU_VIRTUAL_ADDRESS CristerResidency::PositionBufferAddress()const
	{
		return positionResource_ ? positionResource_->GetGPUVirtualAddress() : 0;
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
	Uint CristerResidency::PositionBufferIndex()const
	{
		return positionBufferIndex_;
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
	Uint32 CristerResidency::VertexCount()const
	{
		return proxyVertexCount_;
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
	D3D12_GPU_VIRTUAL_ADDRESS CristerResidency::IndexBufferAddress()const
	{
		return indexResource_ ? indexResource_->GetGPUVirtualAddress() : 0;
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
	Uint32 CristerResidency::IndexCount()const
	{
		return triangleIndexCount_;
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
	Uint CristerResidency::TextureBindlessIndex(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return SC_INVALID;
		}
		const StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		/// [EN] A texture that does not stream, or has nothing resident yet, has no SRV to give.
		/// [JP] ストリーミングしないテクスチャや、まだ何も常駐していないテクスチャには返せる SRV が無い。
		if (!streamingTexture.valid_ || streamingTexture.topResidentMip_ >= streamingTexture.mipCount_)
		{
			return SC_INVALID;
		}

		/// [EN] Return the finest resident mip: the pinned one until something finer has streamed in, then currentMip_.
		/// [JP] 常駐中で最も細かいミップを返す。より細かいものがストリームインするまではピン留めのもの、その後は currentMip_。
		Bool isPinnedMip = streamingTexture.topResidentMip_ == streamingTexture.mipCount_ - 1;
		return isPinnedMip ? streamingTexture.pinnedMip_.bindlessIndex_ : streamingTexture.currentMip_.bindlessIndex_;
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
	Bool CristerResidency::MaterialsDirty()const
	{
		return materialsDirty_;
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
	void CristerResidency::ClearMaterialsDirty()const
	{
		materialsDirty_ = false;
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
	Uint CristerResidency::VertexBufferIndex()const
	{
		return vertexBufferIndex_;
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
	Uint CristerResidency::IndexBufferIndex()const
	{
		return indexBufferIndex_;
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
	Uint CristerResidency::SkinVertexBufferIndex()const
	{
		return skinVertexBufferIndex_;
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
	Uint CristerResidency::MorphDeltaBufferIndex()const
	{
		return morphDeltaBufferIndex_;
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
	Uint CristerResidency::VertexMorphSourceBufferIndex()const
	{
		return vertexMorphSourceBufferIndex_;
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
	Bool CristerResidency::ProxySkinned()const
	{
		return raytracingSkinVertexResource_ != nullptr;
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
	Uint CristerResidency::ProxySkinVertexBufferIndex()const
	{
		return raytracingSkinVertexBufferIndex_;
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
	Uint CristerResidency::ProxyMorphDeltaBufferIndex()const
	{
		return raytracingMorphDeltaBufferIndex_;
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
	std::span<const Matrix> CristerResidency::SubMeshPlacement(Size subMeshIndex)const
	{
		/// [EN] Fallback for an index Upload() has not resolved: one identity placement, i.e. draw at the actor world as-is.
		/// [JP] Upload() が解決していないインデックス用のフォールバック: 単位行列1つ、つまりアクターワールドのまま描く。
		static const DynamicArray<Matrix> identityPlacement = { Matrix::Identity };
		return subMeshIndex < subMeshPlacement_.size() ? subMeshPlacement_[subMeshIndex] : identityPlacement;
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
	void CristerResidency::CopyMorph(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination)const
	{
		/// [EN] Nothing to copy without a proxy, a destination, or any proxy vertices.
		/// [JP] プロキシか書き込み先、またはプロキシ頂点が無ければコピーするものは無い。
		if (!positionResource_ || !destination || proxyVertexCount_ == 0)
		{
			return;
		}

		/// [EN] Copy every base position, one float3 per proxy vertex.
		/// [JP] 全てのベース位置を、プロキシ頂点1つにつき float3 1つぶんコピーする。
		cmdList->CopyBufferRegion(destination, 0, positionResource_.Get(), 0, static_cast<UINT64>(proxyVertexCount_) * sizeof(Vector3));
	}

	/**
	* [EN]
	* Creates the cluster's page (and the vertex pool if the cluster is
	* LOD 0) and records its copies on lane. No-op when already resident.
	*
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
	* クラスタのページを作り（LOD 0 なら頂点プールも）、そのコピーを lane に記録する。
	* 常駐済みなら何もしない。
	*
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
	void CristerResidency::MakeClusterResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap, Uint32 clusterIndex)
	{
		StreamingGeometry& page = streamingGeometry_[clusterIndex];
		const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];

		/// [EN] Nothing to do for a page already in VRAM, an empty cluster, or a Crister that has not run Upload().
		/// [JP] 既に VRAM にあるページ、空のクラスタ、Upload() を通っていない Crister では何もしない。
		if (page.resident_ || cluster.meshletCount_ == 0 || !uploaded_)
		{
			return;
		}

		/// [EN] A page that reads the shared pool needs the pool resident first, and counts as one more reference keeping it there.
		/// [JP] 共有プールを読むページは、先にプールを常駐させ、プールを残しておく参照の1つとして数える。
		if (!page.ownsVertices_)
		{
			MakePoolResident(crister, device, lane, heap);
			poolResidentReferences_++;
		}

		/// [EN] Rebase the cluster's meshlets to page-local offsets. The mesh shaders are untouched: ModelStructuredBuffer simply points at the page's buffers and a page-local meshlet offset.
		/// [JP] クラスタの meshlet をページローカルオフセットへリベースする。メッシュシェーダは無変更: ModelStructuredBuffer がページのバッファとページローカルの meshlet オフセットを指すだけ。
		DynamicArray<MeshletDesc> localMeshlets(cluster.meshletCount_);
		DynamicArray<MeshletBound> localBounds(cluster.meshletCount_);
		for (Uint32 meshletIndex = 0; meshletIndex < cluster.meshletCount_; meshletIndex++)
		{
			MeshletDesc meshlet = crister.geometry_.meshlets_[cluster.meshletOffset_ + meshletIndex];
			meshlet.vertexOffset_ -= page.vertexIndexBegin_;
			meshlet.triangleOffset_ -= page.primitiveBegin_;
			localMeshlets[meshletIndex] = meshlet;
			localBounds[meshletIndex] = crister.geometry_.meshletBounds_[cluster.meshletOffset_ + meshletIndex];
		}

		/// [EN] Copy the page's vertex indices; for a page with its own vertices, rebase them so they index its local vertex slice.
		/// [JP] ページの頂点インデックスをコピーする。自前の頂点を持つページでは、ローカルな頂点スライスを指すようにリベースする。
		DynamicArray<Uint32> localVertexIndices(page.vertexIndexEnd_ - page.vertexIndexBegin_);
		for (Uint32 index = page.vertexIndexBegin_; index < page.vertexIndexEnd_; index++)
		{
			Uint32 value = crister.geometry_.vertexIndices_[index];
			localVertexIndices[index - page.vertexIndexBegin_] = page.ownsVertices_ ? value - page.vertexBegin_ : value;
		}

		/// [EN] The primitive bytes are read through a raw buffer, which addresses whole 32-bit words, so the copy is padded to a multiple of 4 bytes.
		/// [JP] プリミティブのバイト列は 32bit 単位でアクセスする raw バッファとして読むので、コピーを4バイトの倍数まで詰め物する。
		Uint32 primitiveSize = page.primitiveEnd_ - page.primitiveBegin_;
		Uint32 alignedSize = (primitiveSize + 3) & ~3u;
		DynamicArray<Uint8> localPrimitives(alignedSize, 0);
		memcpy(localPrimitives.data(), crister.geometry_.primitiveIndices_.data() + page.primitiveBegin_, primitiveSize);

		/// [EN] A page that owns vertices carries its own copy of its vertex range.
		/// [JP] 頂点を持つページは、自分の頂点範囲のコピーを持つ。
		DynamicArray<CompressedVertex> localVertices;
		if (page.ownsVertices_)
		{
			localVertices.resize(page.vertexEnd_ - page.vertexBegin_);
			for (Uint32 vertexIndex = page.vertexBegin_; vertexIndex < page.vertexEnd_; vertexIndex++)
			{
				localVertices[vertexIndex - page.vertexBegin_] = crister.geometry_.compressedVertices_[vertexIndex];
			}
		}

		/// [EN] Create the page's buffers and SRVs, recording their copies on lane.
		/// [JP] ページのバッファと SRV を作り、そのコピーを lane に記録する。
		HRESULT hr = CreateStaticBufferUnbounded(device, lane, localMeshlets.data(), localMeshlets.size(), sizeof(MeshletDesc), page.meshletResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "メッシュレットバッファの生成に失敗しました");
		page.meshletBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.meshletResource_.Get(), cluster.meshletCount_, sizeof(MeshletDesc));

		hr = CreateStaticBufferUnbounded(device, lane, localBounds.data(), localBounds.size(), sizeof(MeshletBound), page.meshletBoundResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "メッシュレットバウンドバッファの生成に失敗しました");
		page.meshletBoundBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.meshletBoundResource_.Get(), cluster.meshletCount_, sizeof(MeshletBound));

		hr = CreateStaticBufferUnbounded(device, lane, localVertexIndices.data(), localVertexIndices.size(), sizeof(Uint32), page.vertexIndicesResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "頂点インデックスバッファの生成に失敗しました");
		page.vertexIndicesBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.vertexIndicesResource_.Get(), static_cast<Uint>(localVertexIndices.size()), sizeof(Uint32));

		hr = CreateStaticBufferUnbounded(device, lane, localPrimitives.data(), alignedSize, 1, page.primitiveIndicesResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "プリミティブインデックスバッファの生成に失敗しました");
		{
			/// [EN] The primitive indices get a raw (ByteAddressBuffer) SRV over 32-bit words instead of a structured one.
			/// [JP] プリミティブインデックスには構造化バッファではなく、32bit ワード単位の raw (ByteAddressBuffer) の SRV を作る。
			Uint index = heap->AllocateIndex();

			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Buffer.FirstElement = 0;
			shaderResourceViewDesc.Buffer.NumElements = alignedSize / 4;
			shaderResourceViewDesc.Buffer.StructureByteStride = 0;
			shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			device->CreateShaderResourceView(page.primitiveIndicesResource_.Get(), &shaderResourceViewDesc, heap->CPUHandle(index));

			page.primitiveIndicesBufferIndex_ = index;
		}

		if (page.ownsVertices_)
		{
			hr = CreateStaticBufferUnbounded(device, lane, localVertices.data(), localVertices.size(), sizeof(CompressedVertex), page.vertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "頂点バッファの生成に失敗しました");
			page.vertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.vertexResource_.Get(), static_cast<Uint>(localVertices.size()), sizeof(CompressedVertex));
		}

		/// [EN] Count the page's VRAM against the geometry budget.
		/// [JP] ページの VRAM をジオメトリの予算に数える。
		page.sizeBytes_ = static_cast<Uint64>(cluster.meshletCount_) * (sizeof(MeshletDesc) + sizeof(MeshletBound)) + localVertexIndices.size() * sizeof(Uint32) + alignedSize + localVertices.size() * sizeof(CompressedVertex);
		totalResidentGeometryBytes_ += page.sizeBytes_;
		page.resident_ = true;
		++version_;
	}

	/**
	* [EN]
	* Texture mip counterpart to MakeClusterResident: streams a mip level
	* instead of a cluster (see StreamingTexture below). targetMip is
	* uploaded directly rather than one level at a time, so a texture
	* reaches the resolution the camera wants in a single copy recorded on
	* lane instead of one copy per level.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MakeClusterResident のテクスチャミップ版の対。クラスタではなく
	* ミップレベルをストリーミングする（下記 StreamingTexture 参照）。
	* targetMip は1段ずつではなく直接アップロードするので、テクスチャは
	* 段ごとにコピーすることなく、lane に記録する1回のコピーでカメラが
	* 求める解像度に届く。
	*/
	void CristerResidency::MakeTextureMipResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap, Uint32 textureIndex, Uint32 targetMip)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}

		StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		/// [EN] A texture that does not stream, or a Crister that has not run Upload(), cannot take a mip.
		/// [JP] ストリーミングしないテクスチャや、Upload() を通っていない Crister にはミップを入れられない。
		if (!streamingTexture.valid_ || !uploaded_)
		{
			return;
		}

		/// [EN] Nothing to do when an equally fine or finer mip is already resident.
		/// [JP] 既に同等以上に細かいミップが常駐しているなら何もしない。
		Uint32 mipIndex = Min(targetMip, streamingTexture.mipCount_ - 1);
		if (mipIndex >= streamingTexture.topResidentMip_)
		{
			return;
		}

		/// [EN] Find the mip's size, pitches and position within the baked BC7 data.
		/// [JP] 焼き込んだ BC7 データの中での、ミップのサイズ・ピッチ・位置を求める。
		const Bitmap& bitmap = crister.material_.bitmaps_[textureIndex];

		Uint32 mipWidth = 0;
		Uint32 mipHeight = 0;
		Uint64 rowPitch = 0;
		Uint64 slicePitch = 0;
		Uint64 byteOffset = 0;
		ComputeTextureMipLayout(bitmap, mipIndex, mipWidth, mipHeight, rowPitch, slicePitch, byteOffset);

		/// [EN] A single-mip BC7 texture at exactly this mip's resolution.
		/// [JP] ちょうどこのミップの解像度を持つ、単一ミップの BC7 テクスチャ。
		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = mipWidth;
		textureDesc.Height = mipHeight;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		/// [EN] Created in COMMON: the copy queue promotes it to COPY_DEST, it decays back to COMMON once the copy completes, and the direct queue promotes it to a shader-resource state when it is sampled.
		/// [JP] COMMON で作る。Copy キューが COPY_DEST へ自動で上げ、コピーの完了で COMMON に戻り、Direct キューがサンプリングするときにシェーダーリソースの状態へ自動で上げる。
		Microsoft::WRL::ComPtr<ID3D12Resource> newResource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(newResource.ReleaseAndGetAddressOf()));

		/// [EN] A failure on one mip is not worth taking the whole model down: warn and carry on without this mip (topResidentMip_ does not advance).
		/// [JP] ミップ1枚の失敗はモデル全体を巻き込んでクラッシュさせるほどではない。警告を出してこのミップ抜きで続行する(topResidentMip_ は進めない)。
		if (FAILED(hr))
		{
			SC_LOG_WARNING("テクスチャ {} のミップ {} のアップロードに失敗しました(HRESULT=0x{:08X}, {}x{})。このミップ抜きで続行します。", textureIndex, mipIndex, static_cast<Uint32>(hr), mipWidth, mipHeight);
			return;
		}
#if SC_DEBUG
		newResource->SetName(L"Crister_TextureMip");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(newResource.Get());
#endif

		/// [EN] Record the copy of the mip's tightly packed blocks straight from cacheData_.
		/// [JP] 隙間なく詰まったミップのブロックを、cacheData_ から直接コピーするよう記録する。
		std::span<const std::byte> mipBytes = std::as_bytes(std::span(bitmap.cacheData_)).subspan(byteOffset, slicePitch);
		lane.GetStream().Upload(device, lane.GetCommandList(), newResource.Get(), 0, mipBytes);

		/// [EN] Give the new mip its own bindless SRV.
		/// [JP] 新しいミップに専用の bindless SRV を作る。
		Uint newIndex = heap->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_BC7_UNORM;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		device->CreateShaderResourceView(newResource.Get(), &shaderResourceViewDesc, heap->CPUHandle(newIndex));

		/// [EN] Intermediate mips are not kept: only the first upload of the pinned (coarsest) mip goes to pinnedMip_; every other upload replaces currentMip_, releasing the old one first.
		/// [JP] 中間ミップは常駐させ続けない: ピン留めミップ(最粗)を初めてアップロードする1回だけ pinnedMip_ へ、それ以外は currentMip_ を置き換える(古い currentMip_ があれば解放してから差し替える)。
		if (mipIndex == streamingTexture.mipCount_ - 1)
		{
			streamingTexture.pinnedMip_.resource_ = newResource;
			streamingTexture.pinnedMip_.bindlessIndex_ = newIndex;
			streamingTexture.pinnedMip_.sizeBytes_ = slicePitch;
		}
		else
		{
			if (streamingTexture.currentMip_.resource_)
			{
				/// [EN] The mip being replaced is the one the in-flight frames are still sampling - this upgrade is driven by worldScale, so it fires on the very frame the Actor's Scale changes. Dropping the last reference here would destroy the texture out from under the GPU.
				/// [JP] 置き換えられるミップは、インフライトのフレームがまだサンプリングしている当のもの — この昇格は worldScale で駆動されるため、Actor の Scale を変えたまさにそのフレームで発火する。ここで最後の参照を落とすと、GPU が使っている最中にテクスチャを破棄することになる。
				heap->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
				totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
			}
			streamingTexture.currentMip_.resource_ = newResource;
			streamingTexture.currentMip_.bindlessIndex_ = newIndex;
			streamingTexture.currentMip_.sizeBytes_ = slicePitch;
		}

		/// [EN] Count the mip against the texture budget, record it as the finest resident one, and tell the RT material table that indices changed.
		/// [JP] ミップをテクスチャの予算に数え、常駐中で最も細かいものとして記録し、RT のマテリアルテーブルにインデックスが変わったことを知らせる。
		totalResidentTextureBytes_ += slicePitch;
		streamingTexture.topResidentMip_ = mipIndex;
		materialsDirty_ = true;
	}

	/**
	* [EN]
	* Creates the shared LOD 0 vertex pool page and records its copy on
	* lane. No-op when already resident, when poolVertexEnd_ is 0 (no
	* LOD 0 clusters), or when Upload() has not run.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有 LOD 0 頂点プールページを作り、そのコピーを lane に記録する。
	* 常駐済み、poolVertexEnd_ が 0（LOD 0 クラスタが無い）、または
	* Upload() を通っていない場合は何もしない。
	*/
	void CristerResidency::MakePoolResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap)
	{
		/// [EN] Nothing to do when the pool is already in VRAM, no LOD 0 cluster needs it, or Upload() has not run.
		/// [JP] プールが既に VRAM にあるか、どの LOD 0 クラスタも必要としないか、Upload() を通っていなければ何もしない。
		if (poolResident_ || poolVertexEnd_ == 0 || !uploaded_)
		{
			return;
		}

		/// [EN] The pool is the leading [0, poolVertexEnd_) range of compressedVertices_, uploaded as is.
		/// [JP] プールは compressedVertices_ の先頭 [0, poolVertexEnd_) の範囲で、そのままアップロードする。
		HRESULT hr = CreateStaticBufferUnbounded(device, lane, crister.geometry_.compressedVertices_.data(), poolVertexEnd_, sizeof(CompressedVertex), poolResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "頂点プールバッファの生成に失敗しました");
		poolBufferIndex_ = CreateStructuredShaderResourceView(device, heap, poolResource_.Get(), poolVertexEnd_, sizeof(CompressedVertex));

		/// [EN] Count the pool against the geometry budget.
		/// [JP] プールをジオメトリの予算に数える。
		poolSizeBytes_ = static_cast<Uint64>(poolVertexEnd_) * sizeof(CompressedVertex);
		totalResidentGeometryBytes_ += poolSizeBytes_;
		poolResident_ = true;
		++version_;
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
	void CristerResidency::EvictCluster(BindlessHeap* heap, Uint32 clusterIndex)
	{
		StreamingGeometry& page = streamingGeometry_[clusterIndex];
		if (!page.resident_ || page.pinned_)
		{
			return;
		}

		/// [EN] Hand the page's buffers to the deferred-reclaim ring and return their descriptors.
		/// [JP] ページのバッファを遅延回収リングへ渡し、ディスクリプタを返す。
		heap->Release(std::move(page.meshletResource_), page.meshletBufferIndex_);
		heap->Release(std::move(page.meshletBoundResource_), page.meshletBoundBufferIndex_);
		heap->Release(std::move(page.vertexIndicesResource_), page.vertexIndicesBufferIndex_);
		heap->Release(std::move(page.primitiveIndicesResource_), page.primitiveIndicesBufferIndex_);
		if (page.ownsVertices_)
		{
			heap->Release(std::move(page.vertexResource_), page.vertexBufferIndex_);
		}
		else
		{
			/// [EN] A page that read the pool drops its reference, which may let EvictPool() free it.
			/// [JP] プールを読んでいたページは参照を外す。これで EvictPool() がプールを解放できるようになることがある。
			poolResidentReferences_--;
		}

		/// [EN] Take the page out of the geometry total.
		/// [JP] ページをジオメトリの合計から外す。
		totalResidentGeometryBytes_ -= page.sizeBytes_;
		page.sizeBytes_ = 0;
		page.resident_ = false;
		++version_;
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
	void CristerResidency::EvictTextureMip(BindlessHeap* heap, Uint32 textureIndex)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}

		StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		/// [EN] Only currentMip_ can be evicted; a texture already down to its pinned mip has nothing to give back.
		/// [JP] 追い出せるのは currentMip_ だけ。既にピン留めのミップだけになっているテクスチャには返すものが無い。
		if (!streamingTexture.valid_ || streamingTexture.topResidentMip_ >= streamingTexture.mipCount_ - 1)
		{
			return;
		}

		/// [EN] Release currentMip_, take it out of the texture total, and fall back to the pinned mip.
		/// [JP] currentMip_ を解放してテクスチャの合計から外し、ピン留めのミップへ戻す。
		heap->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
		streamingTexture.currentMip_.bindlessIndex_ = SC_INVALID;
		totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
		streamingTexture.currentMip_.sizeBytes_ = 0;
		streamingTexture.topResidentMip_ = streamingTexture.mipCount_ - 1;

		/// [EN] The texture's SRV changed, so the RT material table must be rebuilt.
		/// [JP] テクスチャの SRV が変わったので、RT のマテリアルテーブルを作り直す必要がある。
		materialsDirty_ = true;
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
	void CristerResidency::EvictPool(BindlessHeap* heap)
	{
		/// [EN] The pool may go only when it is resident, not pinned, and no resident page reads it.
		/// [JP] プールを追い出せるのは、常駐していて、ピン留めでなく、どの常駐ページも読んでいないときだけ。
		if (!poolResident_ || poolPinned_ || poolResidentReferences_ > 0)
		{
			return;
		}
		heap->Release(std::move(poolResource_), poolBufferIndex_);
		totalResidentGeometryBytes_ -= poolSizeBytes_;
		poolSizeBytes_ = 0;
		poolResident_ = false;
		++version_;
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
	void CristerResidency::TouchCluster(Uint32 clusterIndex, Uint64 frame)
	{
		streamingGeometry_[clusterIndex].lastUsedFrame_ = frame;

		/// [EN] Drawing a page that reads the pool also counts as using the pool.
		/// [JP] プールを読むページを描画することは、プールを使ったことにもなる。
		if (!streamingGeometry_[clusterIndex].ownsVertices_)
		{
			poolLastUsedFrame_ = frame;
		}
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
	void CristerResidency::TouchTexture(Uint32 textureIndex, Uint64 frame)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}
		streamingTextures_[textureIndex].lastUsedFrame_ = frame;
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
	Bool CristerResidency::ClusterResident(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].resident_;
	}

	/**
	* [EN]
	* Returns the residency version: it goes up by one every time a cluster
	* page or the vertex pool becomes resident or is evicted, so a caller
	* that remembers it can skip re-reading cluster residency while it is
	* unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 常駐の版数を返す。クラスタのページか頂点プールが常駐するか追い出される
	* たびに 1 増えるので、覚えておいた呼び出し側は、変わらない間はクラスタの
	* 常駐を読み直さずに済む。
	*/
	Uint64 CristerResidency::Version()const
	{
		return version_;
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
	Bool CristerResidency::TextureResident(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return false;
		}

		/// [EN] Fully resident means mip 0, the finest, is the one in VRAM.
		/// [JP] 完全に常駐しているとは、最も細かいミップ 0 が VRAM にあること。
		return streamingTextures_[textureIndex].topResidentMip_ == 0;
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
	void CristerResidency::EvictClusterBudget(BindlessHeap* heap, Uint64 currentFrame)
	{
		/// [EN] Nothing to do while the geometry fits its budget.
		/// [JP] ジオメトリが予算内に収まっている間は何もしない。
		if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
		{
			return;
		}

		/// [EN] A page that may be evicted, with the frame it was last used.
		/// [JP] 追い出してよいページと、それが最後に使われたフレーム。
		struct Candidate
		{
			/// [EN] Crister the page belongs to.
			/// [JP] ページが属する Crister。
			CristerResidency* crister_;

			/// [EN] Index of the page's cluster in that Crister.
			/// [JP] その Crister での、ページのクラスタのインデックス。
			Uint32 clusterIndex_;

			/// [EN] Last frame the page was drawn from.
			/// [JP] ページが最後に描画に使われたフレーム。
			Uint64 lastUsedFrame_;
		};

		/// [EN] Collect every resident, unpinned page across all Crister that has gone unused for at least evictAgeFrames_.
		/// [JP] 全 Crister から、常駐中でピン留めでなく、evictAgeFrames_ 以上使われていないページを集める。
		DynamicArray<Candidate> candidates;
		for (CristerResidency* crister : streamingRegistry_)
		{
			for (Size clusterIndex = 0; clusterIndex < crister->streamingGeometry_.size(); clusterIndex++)
			{
				const StreamingGeometry& page = crister->streamingGeometry_[clusterIndex];
				if (page.resident_ && !page.pinned_ && page.lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
				{
					candidates.push_back({ crister, static_cast<Uint32>(clusterIndex), page.lastUsedFrame_ });
				}
			}
		}

		/// [EN] Least recently used first.
		/// [JP] 最後に使われたのが古い順に並べる。
		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
			{
				return a.lastUsedFrame_ < b.lastUsedFrame_;
			});

		/// [EN] Evict pages until the total is back within the budget.
		/// [JP] 合計が予算内に戻るまでページを追い出す。
		for (const Candidate& candidate : candidates)
		{
			if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
			{
				break;
			}
			candidate.crister_->EvictCluster(heap, candidate.clusterIndex_);
		}

		/// [EN] Still over budget: free vertex pools that have gone unused long enough and that no resident page reads any more.
		/// [JP] まだ予算を超えていれば、十分長く使われておらず、もうどの常駐ページも読んでいない頂点プールを解放する。
		for (CristerResidency* crister : streamingRegistry_)
		{
			if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
			{
				break;
			}
			if (crister->poolLastUsedFrame_ + evictAgeFrames_ <= currentFrame)
			{
				crister->EvictPool(heap);
			}
		}
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
	void CristerResidency::EvictTextureBudget(BindlessHeap* heap, Uint64 currentFrame)
	{
		/// [EN] Nothing to do while the textures fit their budget.
		/// [JP] テクスチャが予算内に収まっている間は何もしない。
		if (totalResidentTextureBytes_ <= textureBudgetBytes_)
		{
			return;
		}

		/// [EN] A texture whose currentMip_ may be evicted, with the frame it was last sampled.
		/// [JP] currentMip_ を追い出してよいテクスチャと、それが最後にサンプルされたフレーム。
		struct Candidate
		{
			/// [EN] Crister the texture belongs to.
			/// [JP] テクスチャが属する Crister。
			CristerResidency* crister_;

			/// [EN] Index of the texture in that Crister.
			/// [JP] その Crister でのテクスチャのインデックス。
			Uint32 textureIndex_;

			/// [EN] Last frame the texture was sampled.
			/// [JP] テクスチャが最後にサンプルされたフレーム。
			Uint64 lastUsedFrame_;
		};

		/// [EN] Collect every texture across all Crister holding a mip finer than its pinned one that has gone unused for at least evictAgeFrames_.
		/// [JP] 全 Crister から、ピン留めより細かいミップを持ち、evictAgeFrames_ 以上使われていないテクスチャを集める。
		DynamicArray<Candidate> candidates;
		for (CristerResidency* crister : streamingRegistry_)
		{
			for (Size textureIndex = 0; textureIndex < crister->streamingTextures_.size(); textureIndex++)
			{
				const StreamingTexture& streamingTexture = crister->streamingTextures_[textureIndex];
				if (streamingTexture.valid_ && streamingTexture.topResidentMip_ < streamingTexture.mipCount_ - 1 && streamingTexture.lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
				{
					candidates.push_back({ crister, static_cast<Uint32>(textureIndex), streamingTexture.lastUsedFrame_ });
				}
			}
		}

		/// [EN] Least recently used first.
		/// [JP] 最後に使われたのが古い順に並べる。
		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
			{
				return a.lastUsedFrame_ < b.lastUsedFrame_;
			});

		/// [EN] Drop textures back to their pinned mip until the total is within the budget.
		/// [JP] 合計が予算内に戻るまで、テクスチャをピン留めのミップへ戻していく。
		for (const Candidate& candidate : candidates)
		{
			if (totalResidentTextureBytes_ <= textureBudgetBytes_)
			{
				break;
			}
			candidate.crister_->EvictTextureMip(heap, candidate.textureIndex_);
		}
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
	Uint CristerResidency::ClusterVertexBufferIndex(Uint32 clusterIndex)const
	{
		const StreamingGeometry& page = streamingGeometry_[clusterIndex];

		/// [EN] A page with its own vertices reads them; any other page reads the shared pool.
		/// [JP] 自前の頂点を持つページはそれを読み、それ以外のページは共有プールを読む。
		return page.ownsVertices_ ? page.vertexBufferIndex_ : poolBufferIndex_;
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
	Uint CristerResidency::ClusterMeshletBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].meshletBufferIndex_;
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
	Uint CristerResidency::ClusterMeshletBoundBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].meshletBoundBufferIndex_;
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
	Uint CristerResidency::ClusterVertexIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].vertexIndicesBufferIndex_;
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
	Uint CristerResidency::ClusterPrimitiveIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].primitiveIndicesBufferIndex_;
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
	Bool CristerResidency::StandaloneVertices(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].ownsVertices_;
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
	Uint32 CristerResidency::TextureFinestMip(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return 0;
		}
		return streamingTextures_[textureIndex].topResidentMip_;
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
	Uint32 CristerResidency::TextureDesiredMip(const Crister& crister, Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const
	{
		/// [EN] A texture that does not stream has no mip to choose.
		/// [JP] ストリーミングしないテクスチャには選ぶミップが無い。
		if (textureIndex >= streamingTextures_.size() || !streamingTextures_[textureIndex].valid_)
		{
			return 0;
		}

		/// [EN] With no camera or no projection yet, pixelsPerUnit is 0 and there is nothing to decide by, so the finest mip is requested (falling back to the coarsest would turn every texture into its average color).
		/// [JP] カメラが無い/射影が未設定だと pixelsPerUnit が 0 になる。その場合は判断材料が無いので最も細かいミップを要求する(最粗へ倒すと全テクスチャが平均色になってしまう)。
		Float screenPixelsPerWorldUnit = worldScale * pixelsPerUnit;
		if (!(screenPixelsPerWorldUnit > 0.0f))
		{
			return 0;
		}

		/// [EN] texelsPerWorldUnit: how many mip-0 texels one (mesh-local) world unit of surface spans. With the baked texcoordDensity this is exact on average for the SubMesh - a UV atlas that packs many islands into one texture gets a proportionally higher density than "the texture spans the model once" would give. max(width, height) keeps non-square textures on the sharp side. texelsPerScreenPixel is then how many mip-0 texels crowd into one screen pixel; each doubling costs exactly one mip level (rounded down, toward sharp).
		/// [JP] texelsPerWorldUnit: 表面の(メッシュローカル)1 ワールド単位がmip 0 の何テクセルに相当するか。焼き込んだ texcoordDensity を使うと SubMesh 平均で正確になる — 多数の UV アイランドを 1 枚に詰めたアトラスは、「モデルに 1 回貼られる」前提より相応に密度が高くなる。非正方形テクスチャは max(width, height) で鮮明側に倒す。texelsPerScreenPixel は画面 1 ピクセルに詰め込まれる mip 0 のテクセル数で、2 倍になるごとにちょうど 1 ミップ粗くできる(切り捨てて鮮明側に倒す)。
		const Bitmap& bitmap = crister.material_.bitmaps_[textureIndex];
		Float textureResolution = static_cast<Float>(Max(bitmap.width_, bitmap.height_));

		Float texelsPerWorldUnit = 0.0f;

		/// [EN] Use the SubMesh's baked density; without it, assume the texture spans the model's largest dimension once.
		/// [JP] SubMesh に焼き込んだ密度を使う。無ければ、テクスチャがモデルの最も大きい寸法に1回貼られているとみなす。
		if (texcoordDensity > 0.0f)
		{
			texelsPerWorldUnit = texcoordDensity * textureResolution;
		}
		else
		{
			Vector3 extent = crister.geometry_.positionExtent_;
			Float modelSize = Max(Max(extent.x, extent.y), extent.z);
			texelsPerWorldUnit = textureResolution / Max(modelSize, 1e-6f);
		}

		/// [EN] While a screen pixel covers at most one texel, mip 0 is needed; beyond that, each doubling of texels per pixel allows one coarser mip.
		/// [JP] 画面1ピクセルが覆うテクセルが1つ以下ならミップ 0 が要る。それを超えると、ピクセルあたりのテクセルが2倍になるごとに1段粗いミップでよい。
		Uint32 desiredMip = 0;
		Float texelsPerScreenPixel = texelsPerWorldUnit / screenPixelsPerWorldUnit;
		if (texelsPerScreenPixel > 1.0f)
		{
			desiredMip = static_cast<Uint32>(std::log2(texelsPerScreenPixel));
		}

		/// [EN] Never ask for a mip coarser than the coarsest one that streams.
		/// [JP] ストリーミングできる最も粗いミップより粗いものは求めない。
		Uint32 mipCount = streamingTextures_[textureIndex].mipCount_;
		return mipCount == 0 ? 0 : Min(desiredMip, mipCount - 1);
	}

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
	Uint CristerResidency::CreateStructuredShaderResourceView(ID3D12Device* device, BindlessHeap* heap, ID3D12Resource* resource, Uint elementCount, Uint stride)
	{
		/// [EN] Take a free slot in the bindless heap and describe the whole buffer as elementCount structured elements of stride bytes.
		/// [JP] bindless ヒープの空きスロットを取り、バッファ全体を stride バイトの構造化要素 elementCount 個として記述する。
		Uint index = heap->AllocateIndex();

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Buffer.FirstElement = 0;
		shaderResourceViewDesc.Buffer.NumElements = elementCount;
		shaderResourceViewDesc.Buffer.StructureByteStride = stride;
		shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device->CreateShaderResourceView(resource, &shaderResourceViewDesc, heap->CPUHandle(index));

		return index;
	}

	/**
	* [EN]
	* Creates a DEFAULT-heap buffer of count * stride bytes and records the
	* copy of data into it on lane. The size is bounded only by GPU
	* memory, so a high-poly mesh's flat 32-bit triangle index buffer can
	* go past 128MB. The buffer stays in COMMON: the copy queue promotes
	* it to COPY_DEST, it decays back to COMMON once the copy completes,
	* and the shaders read it through implicit promotion.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* count * stride バイトの DEFAULT ヒープのバッファを作り、data を
	* そこへコピーする処理を lane に記録する。サイズの上限は GPU メモリ
	* だけなので、ハイポリメッシュのフラットな 32bit 三角形インデックス
	* バッファが 128MB を超えても作れる。バッファは COMMON のまま:
	* Copy キューが COPY_DEST へ自動で上げ、コピーの完了で COMMON に戻り、
	* シェーダーは自動の昇格で読む。
	*/
	HRESULT CristerResidency::CreateStaticBufferUnbounded(ID3D12Device* device, D3D12TransferLane& lane, const void* data, Uint64 count, Uint64 stride, ID3D12Resource** outResource)
	{
		/// [EN] A plain DEFAULT-heap buffer of the full size, with no cap.
		/// [JP] 上限なしで、全体のサイズを持つ素の DEFAULT ヒープのバッファ。
		const Uint64 sizeInBytes = count * stride;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#if SC_DEBUG
		resource->SetName(L"Crister_StaticBufferUnbounded");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

		/// [EN] Record the copy on lane; the buffer is filled once the caller submits lane and its fence completes.
		/// [JP] コピーを lane に記録する。バッファが埋まるのは、呼び出し側が lane を投入してそのフェンスが完了したとき。
		lane.GetStream().Upload(device, lane.GetCommandList(), resource.Get(), 0, std::span<const std::byte>(static_cast<const std::byte*>(data), sizeInBytes));

		/// [EN] Hand ownership of the reference to the caller.
		/// [JP] 参照の所有権を呼び出し側へ渡す。
		*outResource = resource.Detach();
		return S_OK;
	}

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
	void CristerResidency::ComputeTextureMipLayout(const Bitmap& bitmap, Uint32 mipIndex, Uint32& outWidth, Uint32& outHeight, Uint64& outRowPitch, Uint64& outSlicePitch, Uint64& outByteOffset)
	{
		Uint64 offset = 0;
		Int mipWidth = bitmap.width_;
		Int mipHeight = bitmap.height_;

		/// [EN] Skip over every finer mip: each takes one 16-byte block per 4x4 texels, and the size halves per level down to 1.
		/// [JP] それより細かい全ミップを飛ばす。各ミップは 4x4 テクセルにつき 16 バイトのブロックを1つ使い、サイズは段ごとに半分になって最小 1 まで下がる。
		for (Uint32 mip = 0; mip < mipIndex; mip++)
		{
			Uint64 blockWidth = Max<Uint64>(1, (static_cast<Uint64>(mipWidth) + 3) / 4);
			Uint64 blockHeight = Max<Uint64>(1, (static_cast<Uint64>(mipHeight) + 3) / 4);
			offset += blockWidth * 16 * blockHeight;
			mipWidth = Max(1, mipWidth / 2);
			mipHeight = Max(1, mipHeight / 2);
		}

		/// [EN] The requested mip: its size, one row of blocks as the row pitch, all block rows as the slice pitch.
		/// [JP] 求めるミップ。サイズと、ブロック1行を行ピッチ、ブロックの全行をスライスピッチとする。
		Uint64 blockWidth = Max<Uint64>(1, (static_cast<Uint64>(mipWidth) + 3) / 4);
		Uint64 blockHeight = Max<Uint64>(1, (static_cast<Uint64>(mipHeight) + 3) / 4);
		outWidth = static_cast<Uint32>(mipWidth);
		outHeight = static_cast<Uint32>(mipHeight);
		outRowPitch = blockWidth * 16;
		outSlicePitch = outRowPitch * blockHeight;
		outByteOffset = offset;
	}
}