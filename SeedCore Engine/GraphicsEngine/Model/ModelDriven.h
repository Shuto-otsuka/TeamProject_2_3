#pragma once
#include <FoundationEngine/Log/Assert.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	/**
	* [EN]
	* previousWorld_ is this instance's own world matrix as of the previous
	* frame - see Model.hlsli's ModelTransform::previous_world_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* previousWorld_ はこのインスタンスの前フレーム時点のワールド行列 —
	* Model.hlsli の ModelTransform::previous_world_ 参照。
	*/
	struct ModelTransform
	{
		Matrix world_;
		Matrix inverseTransposeWorld_;
		Matrix previousWorld_;
	};
	SC_STATIC_ASSERT(ModelTransform, 192, "Model/Model.hlsli");

	/**
	* [EN]
	* Bindless SRVs of this instance's geometry buffers (compressed vertices,
	* meshlets, meshlet bounds, meshlet-local vertex indices, packed
	* primitive indices), plus meshletOffset_/meshletCount_ - this
	* instance's range into the shared meshlet buffers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このインスタンスのジオメトリバッファ(圧縮頂点、meshlet、meshlet境界、
	* meshletローカル頂点インデックス、パック済みプリミティブインデックス)の
	* bindless SRVと、meshletOffset_/meshletCount_(共有meshletバッファへの
	* このインスタンスの範囲)。
	*/
	struct ModelGeometry
	{
		Uint vertexBufferIndex_;
		Uint meshletBufferIndex_;
		Uint meshletBoundBufferIndex_;
		Uint vertexIndicesBufferIndex_;
		Uint primitiveIndicesBufferIndex_;
		Uint meshletOffset_;
		Uint meshletCount_;
		Uint layer_;
	};
	SC_STATIC_ASSERT(ModelGeometry, 32, "Model/Model.hlsli");

	/**
	* [EN]
	* Dequantisation AABB for the compressed vertex buffer (Crister::
	* PositionMin etc. - see Model.hlsli's DecodeModelVertex), plus the
	* screen-space error in pixels of this cluster as seen from the closest
	* view, and the cluster's level in its LOD chain (0 = most detailed).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 圧縮頂点バッファの逆量子化AABB(Crister::PositionMinほか — Model.hlsli の
	* DecodeModelVertex 参照)と、いちばん近いビューから見たこのクラスタの
	* 画面上の誤差（ピクセル）、LOD の段の中での段（0 が最も詳細）。
	*/
	struct ModelStreaming
	{
		Vector3 positionMin_;
		Float texcoordMinU_;

		Vector3 positionExtent_;
		Float texcoordMinV_;

		Vector2 texcoordExtent_;
		Float lodError_;
		Uint lodLevel_;
	};
	SC_STATIC_ASSERT(ModelStreaming, 48, "Model/Model.hlsli");

	/**
	* [EN]
	* Per-instance shading parameters copied from the instance's material
	* (Surface) and its actor (3 rows / 48 bytes). shadingModel_ selects the
	* lighting response DeferredLightingPS.hlsl evaluates this instance with
	* (see CristerMaterial.h's ShadingModel); furLength_/furDensity_/
	* furShellCount_ are ShadingModel::Fur tuning. doubleSided_/blend_ route
	* the instance to the culling lists and passes that match its faces and
	* alpha mode, and selected_ marks an actor the editor has selected.
	* materialMapping_/tiling_/triplanarSharpness_ decide how the material's
	* textures are placed (MaterialMapping.hlsli), and actor_ is the entity
	* index of the actor the instance belongs to, which the outline looks its
	* style up by (OutlineCS.hlsl).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* インスタンスのマテリアル(Surface)とアクターから写す、インスタンス
	* ごとのシェーディングの値(3 行 / 48 バイト)。shadingModel_ は
	* DeferredLightingPS.hlsl がこのインスタンスをどのライティング応答で
	* 評価するか(CristerMaterial.h の ShadingModel 参照)、furLength_/
	* furDensity_/furShellCount_ は ShadingModel::Fur のチューニング値。
	* doubleSided_/blend_ は、面の向きとアルファモードに合ったカリングの
	* リストとパスへインスタンスを振り分け、selected_ はエディタで選択中の
	* アクターであることを示す。materialMapping_/tiling_/
	* triplanarSharpness_ はマテリアルのテクスチャの貼り方
	* (MaterialMapping.hlsli)を決め、actor_ はインスタンスが属するアクターの
	* エンティティ番号で、縁取りはこれで設定を引く(OutlineCS.hlsl)。
	*/
	struct ModelShading
	{
		/// [EN] Lighting response the deferred lighting pass evaluates this instance with (ShadingModel); KHR_materials_unlit forces Unlit.
		/// [JP] デフォードライティングパスがこのインスタンスを評価するライティング応答(ShadingModel)。KHR_materials_unlit なら Unlit に固定される。
		Uint shadingModel_;

		/// [EN] Fur shell height in metres (ShadingModel::Fur).
		/// [JP] 毛のシェルの高さ(メートル、ShadingModel::Fur)。
		Float furLength_;

		/// [EN] Per-strand hash-noise density of the fur (ShadingModel::Fur).
		/// [JP] 毛 1 本ごとのハッシュノイズの密度(ShadingModel::Fur)。
		Float furDensity_;

		/// [EN] Number of shell layers drawn above the base surface (ShadingModel::Fur).
		/// [JP] ベース面の上に描くシェルの層数(ShadingModel::Fur)。
		Uint furShellCount_;

		/// [EN] 1 when the material draws both faces, so culling puts the instance in the double-sided list and skips back-face rejection.
		/// [JP] マテリアルが両面を描くときに 1。カリングはインスタンスを両面用のリストへ入れ、裏面の除外を行わない。
		Uint doubleSided_;

		/// [EN] 1 when the material's alpha mode is BLEND, so the instance is drawn by the transparent (OIT) pass instead of the opaque ones.
		/// [JP] マテリアルのアルファモードが BLEND のときに 1。インスタンスは不透明のパスではなく、半透明(OIT)のパスで描かれる。
		Uint blend_;

		/// [EN] 1 when the instance's actor is selected in the editor.
		/// [JP] インスタンスのアクターがエディタで選択されているときに 1。
		Uint selected_;

		/// [EN] How the material's textures are placed (MaterialMapping as an integer, MATERIAL_MAPPING_* in Model.hlsli).
		/// [JP] マテリアルのテクスチャの貼り方(MaterialMapping を整数にしたもの。Model.hlsli の MATERIAL_MAPPING_*)。
		Uint materialMapping_;

		/// [EN] Pattern repetitions per unit of scaled size; unused by MaterialMapping::Stretch.
		/// [JP] スケール込みの大きさ 1 単位あたりの柄の繰り返し回数。MaterialMapping::Stretch では使わない。
		Float tiling_;

		/// [EN] Exponent applied to the normal before it weights the three Triplanar projections.
		/// [JP] 3 つの Triplanar 投影の重みにする前に、法線へ掛ける指数。
		Float triplanarSharpness_;

		/// [EN] Entity index of the actor the instance belongs to, which indexes Outline.Styles; SC_INVALID for an instance with no actor (primitive solids).
		/// [JP] インスタンスが属するアクターのエンティティ番号。Outline.Styles の添え字になる。アクターを持たないインスタンス(プリミティブの形)では SC_INVALID。
		Uint actor_;

		Uint modelShadingPadding0_;
	};
	SC_STATIC_ASSERT(ModelShading, 48, "Model/Model.hlsli");

	/**
	* [EN]
	* skinVertexBufferIndex_ is the bindless SRV of the CompressedModelSkin
	* buffer (0xFFFFFFFF for a static, unskinned instance).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* skinVertexBufferIndex_ は CompressedModelSkin バッファの bindless SRV
	* (静的な非スキンインスタンスでは 0xFFFFFFFF)。
	*/
	struct ModelSkining
	{
		Uint skinIndex_;
		Uint boneOffset_;
		Uint skinVertexBufferIndex_;
		Uint modelSkiningPadding0_;
	};
	SC_STATIC_ASSERT(ModelSkining, 16, "Model/Model.hlsli");

	/**
	* [EN]
	* Raster morph blend (see the model mesh shaders and
	* Model/Material/MaterialResolveCS.hlsl/
	* Model/Transparent/TransparentMS.hlsl). morphDeltaOffset_
	* is in float3 units into morphDeltaBufferIndex_; morphVertexOffset_/
	* morphVertexCount_ are this SubMesh's Crister::vertices_ range start and
	* ORIGINAL (pre-LOD) vertex count; morphWeightOffset_ indexes into the
	* shared per-frame morph weight buffer. morphTargetCount_ == 0 disables
	* blending entirely - always the case for a non-morphed instance, and
	* also for a morphed instance drawn from an own-page (streamed-in,
	* non-pool) cluster: only the LOD 0 shared vertex pool range is
	* index-aligned with morphDeltaBufferIndex_/vertexMorphSourceBufferIndex_'s
	* crister-wide numbering (see Crister::vertexMorphSource_'s comment), so
	* ModelRenderer leaves these zeroed for any other cluster.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ラスタのモーフブレンド(モデル用メッシュシェーダーと
	* Model/Material/MaterialResolveCS.hlsl/
	* Model/Transparent/TransparentMS.hlsl 参照)。
	* morphDeltaOffset_ は morphDeltaBufferIndex_ への float3 単位のオフセット。
	* morphVertexOffset_/morphVertexCount_ はこの SubMesh の
	* Crister::vertices_ 範囲の開始位置と元(LOD前)頂点数。
	* morphWeightOffset_ は共有の毎フレームモーフ重みバッファへのインデックス。
	* morphTargetCount_ == 0 でブレンドを完全に無効化する — モーフ無し
	* インスタンスでは常にこの状態。モーフ付きインスタンスでも、自前ページ
	* (ストリームイン済み、プール外)クラスタから描画される場合も同様 —
	* LOD 0 共有頂点プール範囲だけが morphDeltaBufferIndex_/
	* vertexMorphSourceBufferIndex_ の Crister 全体の番号付けと整合するため
	* (Crister::vertexMorphSource_ のコメント参照)、ModelRenderer は
	* それ以外のクラスタではこれらをゼロのままにする。
	*/
	struct ModelMorphing
	{
		Uint morphDeltaBufferIndex_;
		Uint vertexMorphSourceBufferIndex_;
		Uint morphDeltaOffset_;
		Uint morphVertexOffset_;

		Uint morphVertexCount_;
		Uint morphTargetCount_;
		Uint morphWeightOffset_;
		Uint modelMorphPadding0_;
	};
	SC_STATIC_ASSERT(ModelMorphing, 32, "Model/Model.hlsli");

	/**
	* [EN]
	* Base PBR values and their matching bindless texture SRVs. ior_ is
	* KHR_materials_ior; emissiveStrength_ is KHR_materials_emissive_strength.
	* All four base textures plus occlusionTextureIndex_ are sampled by
	* Model/Opaque/DeferredLightingPS.hlsl using the model UV recovered from
	* the VisibilityBuffer (RT4.zw) - the deferred pass has no interpolated
	* texcoord of its own.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 基本PBR値と、それに対応する bindless テクスチャ SRV。ior_ は
	* KHR_materials_ior、emissiveStrength_ は KHR_materials_emissive_strength。
	* 基本テクスチャ4種と occlusionTextureIndex_ は
	* Model/Opaque/DeferredLightingPS.hlsl がサンプルする。モデルの UV は
	* VisibilityBuffer(RT4.zw)から取る — deferred パスは自前の補間済み
	* texcoord を持たないため。
	*/
	struct ModelTexture
	{
		Color baseColor_;
		Float metallic_;
		Float roughness_;
		Float alphaCutoff_;
		Float ior_;

		Vector3 emissive_;
		Float emissiveStrength_;

		Uint baseColorTextureIndex_;
		Uint normalTextureIndex_;
		Uint metallicRoughnessTextureIndex_;
		Uint emissiveTextureIndex_;

		Uint occlusionTextureIndex_;
		Vector3 modelTexturePadding0_;
	};
	SC_STATIC_ASSERT(ModelTexture, 80, "Model/Model.hlsli");

	/**
	* [EN]
	* KHR extension material values/textures (specular, clearcoat,
	* anisotropy, transmission, volume, sheen, iridescence, unlit). Each
	* texture multiplies its matching factor per the glTF spec's channel
	* convention; 0xFFFFFFFF means absent.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* KHR拡張マテリアルの値/テクスチャ(specular, clearcoat, anisotropy,
	* transmission, volume, sheen, iridescence, unlit)。各テクスチャは
	* glTF仕様のチャンネル規約に従って対応するfactorに乗算される。
	* 0xFFFFFFFFは未設定を意味する。
	*/
	struct ModelExtension
	{
		Float specularFactor_;
		Vector3 specularColor_;
		Float clearCoatFactor_;
		Float clearCoatRoughness_;
		Float anisotropy_;
		Float anisotropyRotation_;
		Float transmissionFactor_;
		Float volumeThicknessFactor_;
		Float volumeAttenuationDistance_;
		Float unlit_;
		Vector3 volumeAttenuationColor_;
		Float sheenRoughness_;
		Vector3 sheenColor_;
		Float iridescenceFactor_;
		Float iridescenceIor_;
		Float iridescenceThickness_;
		Uint specularTextureIndex_;
		Uint specularColorTextureIndex_;
		Uint clearCoatTextureIndex_;
		Uint clearCoatRoughnessTextureIndex_;
		Uint clearCoatNormalTextureIndex_;
		Uint transmissionTextureIndex_;
		Uint thicknessTextureIndex_;
		Uint sheenColorTextureIndex_;
		Uint sheenRoughnessTextureIndex_;
		Uint iridescenceTextureIndex_;
		Uint iridescenceThicknessTextureIndex_;
		Uint anisotropyTextureIndex_;
		Vector2 modelExtensionPadding0_;
	};
	SC_STATIC_ASSERT(ModelExtension, 144, "Model/Model.hlsli");

	/**
	* [EN]
	* Per-instance StructuredBuffer element read by every Model pass (depth
	* prepass, shadow, G-buffer, deferred lighting, GI/reflection/refraction
	* raytracing, OIT). Mirrors Model.hlsli's ModelStructuredBuffer -
	* grouped by concern so the cheap, frequently-run passes (depth/shadow/
	* culling) only ever touch transform_/geometry_/streaming_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全Modelパス(深度プリパス、シャドウ、Gバッファ、ディファードライティング、
	* GI/反射/屈折のレイトレ、OIT)が読む per-instance の StructuredBuffer要素。
	* Model.hlsli の ModelStructuredBuffer と一致 — 関心事ごとにグループ化
	* してあり、軽く頻度の高いパス(深度/シャドウ/カリング)は
	* transform_/geometry_/streaming_ しか触らない。
	*/
	struct ModelStructuredBuffer
	{
		ModelTransform transform_;
		ModelGeometry geometry_;
		ModelStreaming streaming_;
		ModelShading shading_;
		ModelSkining skining_;
		ModelMorphing morph_;
		ModelTexture texture_;
		ModelExtension extension_;
	};
	SC_STATIC_ASSERT(ModelStructuredBuffer, 592, "Model/Model.hlsli");

	struct ModelUnitStructuredBuffer
	{
		Matrix world_;

		Matrix previousWorld_;

		Uint meshOffset_ = 0;

		Uint meshCount_ = 0;

		Uint materialOffset_ = 0;

		Uint flags_ = 0;

		Uint boneOffset_ = SC_INVALID;

		Uint morphWeightOffset_ = SC_INVALID;

		Uint modelEntityStructuredBufferPadding0_ = 0;

		Uint modelEntityStructuredBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(ModelUnitStructuredBuffer, 160, "Model/Model.hlsli");

	struct ModelMeshStructuredBuffer
	{
		Vector3 positionMin_;
		Float texcoordMinU_ = 0.0f;

		Vector3 positionExtent_;
		Float texcoordMinV_ = 0.0f;

		Vector2 texcoordExtent_;
		Float texcoordDensity_ = 0.0f;
		Uint surfaceIndex_ = 0;

		Uint clusterOffset_ = 0;
		Uint clusterCount_ = 0;
		Uint placementOffset_ = 0;
		Uint placementCount_ = 0;

		Uint skinIndex_ = SC_INVALID;
		Uint skinBoneOffset_ = 0;
		Uint skinVertexBufferIndex_ = SC_INVALID;
		Uint morphDeltaBufferIndex_ = SC_INVALID;

		Uint vertexMorphSourceBufferIndex_ = SC_INVALID;
		Uint morphDeltaOffset_ = 0;
		Uint vertexOffset_ = 0;
		Uint vertexCount_ = 0;

		Uint morphTargetCount_ = 0;
		Uint morphWeightOffset_ = 0;
		Uint modelMeshStructuredBufferPadding0_ = 0;
		Uint modelMeshStructuredBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(ModelMeshStructuredBuffer, 112, "Model/Model.hlsli");

	struct ModelClusterStructuredBuffer
	{
		Uint meshletOffset_ = 0;
		Uint meshletCount_ = 0;
		Float lodError_ = 0.0f;
		Uint flags_ = 0;

		Uint vertexBufferIndex_ = SC_INVALID;
		Uint meshletBufferIndex_ = SC_INVALID;
		Uint meshletBoundBufferIndex_ = SC_INVALID;
		Uint vertexIndicesBufferIndex_ = SC_INVALID;

		Uint primitiveIndicesBufferIndex_ = SC_INVALID;
		Uint modelClusterStructuredBufferPadding0_ = 0;
		Uint modelClusterStructuredBufferPadding1_ = 0;
		Uint modelClusterStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(ModelClusterStructuredBuffer, 48, "Model/Model.hlsli");

	struct ModelMaterialStructuredBuffer
	{
		ModelShading shading_;

		ModelTexture texture_;

		ModelExtension extension_;
	};
	SC_STATIC_ASSERT(ModelMaterialStructuredBuffer, 272, "Model/Model.hlsli");

	struct ModelUnitUpdate
	{
		Uint index_ = 0;
		Uint modelEntityUpdatePadding0_ = 0;
		Uint modelEntityUpdatePadding1_ = 0;
		Uint modelEntityUpdatePadding2_ = 0;

		ModelUnitStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelUnitUpdate, 176, "Model/Model.hlsli");

	struct ModelMeshUpdate
	{
		Uint index_ = 0;
		Uint modelMeshUpdatePadding0_ = 0;
		Uint modelMeshUpdatePadding1_ = 0;
		Uint modelMeshUpdatePadding2_ = 0;

		ModelMeshStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelMeshUpdate, 128, "Model/Model.hlsli");

	struct ModelClusterUpdate
	{
		Uint index_ = 0;
		Uint modelClusterUpdatePadding0_ = 0;
		Uint modelClusterUpdatePadding1_ = 0;
		Uint modelClusterUpdatePadding2_ = 0;

		ModelClusterStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelClusterUpdate, 64, "Model/Model.hlsli");

	struct ModelPlacementUpdate
	{
		Uint index_ = 0;
		Uint modelPlacementUpdatePadding0_ = 0;
		Uint modelPlacementUpdatePadding1_ = 0;
		Uint modelPlacementUpdatePadding2_ = 0;

		Matrix data_;
	};
	SC_STATIC_ASSERT(ModelPlacementUpdate, 80, "Model/Model.hlsli");

	struct ModelMaterialUpdate
	{
		Uint index_ = 0;
		Uint modelMaterialUpdatePadding0_ = 0;
		Uint modelMaterialUpdatePadding1_ = 0;
		Uint modelMaterialUpdatePadding2_ = 0;

		ModelMaterialStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelMaterialUpdate, 288, "Model/Model.hlsli");

	struct ModelRaytracingStructuredBuffer
	{
		Uint64 bottomLevel_ = 0;
		Uint vertexBufferIndex_ = SC_INVALID;
		Uint indexBufferIndex_ = SC_INVALID;

		Uint surfaceBufferIndex_ = SC_INVALID;
		Uint modelRaytracingStructuredBufferPadding0_ = 0;
		Vector2 texcoordMin_ = { 0.0f, 0.0f };

		Vector2 texcoordExtent_ = { 0.0f, 0.0f };
		Uint modelRaytracingStructuredBufferPadding1_ = 0;
		Uint modelRaytracingStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(ModelRaytracingStructuredBuffer, 48, "Model/Model.hlsli");

	struct ModelRaytracingUpdate
	{
		Uint index_ = 0;
		Uint modelRaytracingUpdatePadding0_ = 0;
		Uint modelRaytracingUpdatePadding1_ = 0;
		Uint modelRaytracingUpdatePadding2_ = 0;

		ModelRaytracingStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(ModelRaytracingUpdate, 64, "Model/Model.hlsli");

	struct ModelDeformStructuredBuffer
	{
		Uint unitIndex_ = 0;
		Uint subMeshIndex_ = 0;
		Uint vertexOffset_ = 0;
		Uint vertexCount_ = 0;

		Uint outputOffset_ = 0;
		Uint morphDeltaOffset_ = 0;
		Uint morphTargetCount_ = 0;
		Uint positionBufferIndex_ = SC_INVALID;

		Uint skinVertexBufferIndex_ = SC_INVALID;
		Uint morphDeltaBufferIndex_ = SC_INVALID;
		Uint64 bottomLevel_ = 0;
	};
	SC_STATIC_ASSERT(ModelDeformStructuredBuffer, 48, "Model/Model.hlsli");

	struct ModelExpandConstantBuffer
	{
		StaticArray<Vector4, RenderViewCount * 6> planes_;

		StaticArray<Vector4, RenderViewCount> cameras_;

		Uint viewCount_ = 0;
		Uint unitCount_ = 0;
		Uint transparentBase_ = 0;
		Uint capacity_ = 0;

		Uint softbodyOpaqueCount_ = 0;
		Uint softbodyTransparentCount_ = 0;
		Uint modelExpandConstantBufferPadding0_ = 0;
		Uint modelExpandConstantBufferPadding1_ = 0;
	};
	SC_STATIC_ASSERT(ModelExpandConstantBuffer, 368, "Model/Model.hlsli");
}
