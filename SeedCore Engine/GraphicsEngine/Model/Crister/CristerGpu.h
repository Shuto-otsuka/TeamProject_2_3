#pragma once

/**
* [EN]
* CristerGpu streams geometry and texture mips the same way: the coarsest
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
* CristerGpu はジオメトリとテクスチャのミップを同じ形でストリーミングする。
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
	class Crister;
	class BindlessHeap;
	class D3D12CommandQueue;
	struct Bitmap;

	/**
	* [EN]
	* GPU side of a Crister: the RT proxy and morph buffers created at
	* Upload(), the streaming geometry pages and texture mips with their
	* bindless indices, and the budget eviction shared by every uploaded
	* Crister. It reads the model's data through the Crister passed to
	* Upload() and to the residency functions, and owns no CPU-side model
	* data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister の GPU 側。Upload() で作る RT プロキシとモーフのバッファ、
	* ストリーミングするジオメトリのページとテクスチャのミップとその
	* bindless インデックス、アップロード済みの全 Crister で共有する予算
	* による追い出しを持つ。モデルのデータは Upload() と常駐の関数に
	* 渡される Crister から読み、CPU 側のモデルのデータは持たない。
	*/
	class SEEDCORE_API CristerGpu
	{
	public:
		/**
		* [EN]
		* Creates an empty GPU state; Upload() fills it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空の GPU 状態を作る。Upload() で中身を埋める。
		*/
		CristerGpu() = default;

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
		~CristerGpu();

		/**
		* [EN]
		* Moves the GPU resources and streaming state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GPU リソースとストリーミング状態を移動する。
		*/
		CristerGpu(CristerGpu&&)noexcept = default;

		/**
		* [EN]
		* Move-assigns the GPU resources and streaming state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GPU リソースとストリーミング状態をムーブ代入する。
		*/
		CristerGpu& operator=(CristerGpu&&)noexcept = default;

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
		void Upload(Crister& crister, ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap);

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
		* Synchronously uploads the cluster's page (and the vertex pool if
		* the cluster is LOD 0). No-op when already resident.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クラスタのページを同期アップロードする（LOD 0 なら頂点プールも）。
		* 常駐済みなら何もしない。
		*/
		void MakeClusterResident(const Crister& crister, Uint32 clusterIndex);

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
		void MakeTextureMipResident(const Crister& crister, Uint32 textureIndex, Uint32 targetMip);

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
		void MakePoolResident(const Crister& crister);

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
		NODISCARD Uint32 TextureDesiredMip(const Crister& crister, Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const;

	private:
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
		static inline DynamicArray<CristerGpu*> streamingRegistry_;

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