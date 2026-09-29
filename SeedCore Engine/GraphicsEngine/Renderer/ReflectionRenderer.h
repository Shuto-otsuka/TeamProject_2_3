#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/RaytracingDispatch.h>
#include <GraphicsEngine/Raytracing/Reflection/ReflectionDenoiseShader.h>
#include <GraphicsEngine/Raytracing/Reflection/ReflectionShader.h>

namespace SeedCore
{
	struct RootAddresses;

	class BindlessHeap;
	class ConstantIndicesSystem;
	class D3D12CommandList;
	class ShaderCache;
	class ShaderResourceIndicesSystem;
	class UnorderedAccessIndicesSystem;

	/**
	* [EN]
	* Tuning values of the reflection pass. Mirrors ReflectionRayConstantBuffer
	* in Raytracing/Reflection/Reflection.hlsli, which both ReflectionRT.hlsl
	* and DeferredLightingPS.hlsl read through constant_indices.reflection_index_,
	* so the layout must match the HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 反射のパスの調整値。Raytracing/Reflection/Reflection.hlsli の
	* ReflectionRayConstantBuffer と対応する。ReflectionRT.hlsl と
	* DeferredLightingPS.hlsl の両方が constant_indices.reflection_index_ 経由で
	* 読むため、レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct ReflectionRayConstantBuffer
	{
		/// [EN] Maximum distance a reflection ray travels before it counts as a miss.
		/// [JP] 反射レイがミスとして扱われるまでに進む最大距離。
		Float rayTMax_ = 1000.0f;

		/// [EN] Offset along the surface normal applied to each ray origin, so a ray does not hit the surface it leaves.
		/// [JP] 各レイの原点に法線方向へ加えるオフセット。出発した面にレイがヒットしないようにする。
		Float normalBias_ = 0.01f;

		/// [EN] Overall reflection intensity applied in DeferredLightingPS.hlsl.
		/// [JP] DeferredLightingPS.hlsl で適用する反射の全体強度。
		Float strength_ = 1.0f;

		/// [EN] Frame counter written by ReflectionRenderer::Prepare, not by the editor UI. It rotates the GGX importance sample, so the roughness-driven one-sample noise averages out over time instead of forming a fixed pattern.
		/// [JP] エディターの UI ではなく ReflectionRenderer::Prepare が書き込むフレームカウンター。GGX の重点サンプルを回し、roughness に起因する 1 サンプルのノイズが固定の模様にならず時間方向に平均化されるようにする。
		Uint32 frameIndex_ = 0;

		/// [EN] 1 while the reservoir reuses its history across frames; Prepare writes 0 when DLSS Ray Reconstruction does the temporal denoising instead.
		/// [JP] reservoir がフレームをまたいで履歴を再利用する間 1。DLSS Ray Reconstruction が時間方向のデノイズを担うときは Prepare が 0 を書く。
		Uint32 temporalReuseEnabled_ = 1;

		/// [EN] Pads the structure to two full 16-byte registers.
		/// [JP] 構造体を 16 バイトのレジスタ 2 つ分に揃える詰め物。
		Vector3 reflectionRayPadding_ = { 0.0f, 0.0f, 0.0f };

		/**
		* [EN]
		* Reads or writes the tuning values through archive, one field at a
		* time, so a missing or unreadable field falls back to its own default
		* while the others still load. The values written by the renderer
		* every frame and the padding are not serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値をフィールドごとに読み書きする。見つからない、
		* または読めないフィールドはそのフィールドだけ既定値になり、他は通常
		* どおり読み込まれる。レンダラーが毎フレーム書き込む値と詰め物は
		* シリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("rayTMax", rayTMax_);
			archive.TryField("normalBias", normalBias_);
			archive.TryField("strength", strength_);
		}
	};

	/**
	* [EN]
	* One material slot of a mesh's Crister::Surfaces() list, as read by the
	* ray-traced passes. Mirrors ReflectionMaterialData in Reflection.hlsli
	* and is uploaded once per unique Crister
	* (RaytracingRenderer::BuildReflectionMaterialTable).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングのパスが読む、メッシュの Crister::Surfaces() 一覧の
	* マテリアル 1 枠。Reflection.hlsli の ReflectionMaterialData と対応し、
	* ユニークな Crister ごとに 1 度だけアップロードする
	* （RaytracingRenderer::BuildReflectionMaterialTable）。
	*/
	struct ReflectionMaterialData
	{
		/// [EN] Base color factor (rgb).
		/// [JP] ベースカラーの係数（rgb）。
		Float baseColor_[3] = { 1.0f, 1.0f, 1.0f };

		/// [EN] Bindless index of the base color texture, or 0xFFFFFFFF for none.
		/// [JP] ベースカラーテクスチャの bindless インデックス。無ければ 0xFFFFFFFF。
		Uint32 baseColorTextureIndex_ = 0xFFFFFFFF;

		/// [EN] Index of refraction (KHR_materials_ior). Unused by Reflection itself; RefractionRT.hlsl reads it through the same per-triangle table.
		/// [JP] 屈折率（KHR_materials_ior）。Reflection 自体は使わず、RefractionRT.hlsl が同じ三角形単位のテーブル経由で読む。
		Float ior_ = 1.5f;

		/// [EN] Transmission factor (KHR_materials_transmission), read by RefractionRT.hlsl.
		/// [JP] 透過の係数（KHR_materials_transmission）。RefractionRT.hlsl が読む。
		Float transmissionFactor_ = 0.0f;

		/// [EN] Attenuation color of the volume (KHR_materials_volume), read by RefractionRT.hlsl.
		/// [JP] ボリュームの減衰色（KHR_materials_volume）。RefractionRT.hlsl が読む。
		Float volumeAttenuationColor_[3] = { 1.0f, 1.0f, 1.0f };

		/// [EN] Attenuation distance of the volume (KHR_materials_volume); infinity means no absorption.
		/// [JP] ボリュームの減衰距離（KHR_materials_volume）。無限大なら吸収しない。
		Float volumeAttenuationDistance_ = FLT_MAX;

		/// [EN] glTF alphaMode: 0 OPAQUE, 1 MASK, 2 BLEND. Read by IsMaterialPassthrough in Material.hlsli.
		/// [JP] glTF の alphaMode。0 が OPAQUE、1 が MASK、2 が BLEND。Material.hlsli の IsMaterialPassthrough が読む。
		Uint32 alphaMode_ = 0;

		/// [EN] glTF alphaCutoff used in MASK mode.
		/// [JP] MASK モードで使う glTF の alphaCutoff。
		Float alphaCutoff_ = 0.5f;

		/// [EN] Alpha of the glTF baseColorFactor.
		/// [JP] glTF の baseColorFactor のアルファ。
		Float baseColorAlpha_ = 1.0f;

		/// [EN] Volume thickness (KHR_materials_volume). 0 means thin-walled, for which RefractionRT.hlsl skips Beer-Lambert absorption.
		/// [JP] ボリュームの厚み（KHR_materials_volume）。0 は薄い壁を意味し、RefractionRT.hlsl はその場合 Beer-Lambert の吸収を省く。
		Float thicknessFactor_ = 0.0f;

		/// [EN] Bindless index of the thickness texture, or 0xFFFFFFFF for none.
		/// [JP] 厚みテクスチャの bindless インデックス。無ければ 0xFFFFFFFF。
		Uint32 thicknessTextureIndex_ = 0xFFFFFFFF;

		/// [EN] Pads the structure to a whole number of 16-byte rows.
		/// [JP] 構造体を 16 バイトの行の整数倍に揃える詰め物。
		Float materialPadding_ = 0.0f;
	};

	/**
	* [EN]
	* One TLAS instance as seen by the ray-traced passes, in the same order as
	* the TLAS and looked up by InstanceID() in the closest-hit shader.
	* Mirrors ReflectionInstanceData in Reflection.hlsli; it lives in a
	* structured buffer, so it is tightly packed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングのパスから見た TLAS のインスタンス 1 つ。TLAS と同じ
	* 順序で並び、最近接ヒットシェーダーが InstanceID() で引く。
	* Reflection.hlsli の ReflectionInstanceData と対応し、構造化バッファに
	* 入るため詰めて配置する。
	*/
	struct ReflectionInstanceData
	{
		/// [EN] Bindless index of the instance's vertex buffer.
		/// [JP] インスタンスの頂点バッファの bindless インデックス。
		Uint32 vertexBufferIndex_ = 0;

		/// [EN] Bindless index of the instance's index buffer.
		/// [JP] インスタンスのインデックスバッファの bindless インデックス。
		Uint32 indexBufferIndex_ = 0;

		/// [EN] Bindless read view of StructuredBuffer<ReflectionMaterialData>: the mesh's whole material list, resolved per hit triangle through triangleMaterialIndexBufferIndex_ (see ResolveReflectionMaterial in Reflection.hlsli).
		/// [JP] StructuredBuffer<ReflectionMaterialData> の bindless 読み取りビュー。メッシュの全マテリアル一覧で、triangleMaterialIndexBufferIndex_ を通してヒットした三角形ごとに解決する（Reflection.hlsli の ResolveReflectionMaterial 参照）。
		Uint32 materialDataIndex_ = 0;

		/// [EN] Bindless read view of StructuredBuffer<uint> with one entry per triangle, mapping PrimitiveIndex() to an entry of the material list. It lets a multi-material mesh resolve the exact material of the triangle a ray hit rather than one material for the whole instance.
		/// [JP] 三角形 1 つにつき 1 要素の StructuredBuffer<uint> の bindless 読み取りビュー。PrimitiveIndex() をマテリアル一覧の要素へ対応付ける。これにより複数マテリアルのメッシュでも、インスタンス全体で 1 つではなく、レイが当たった三角形そのもののマテリアルを解決できる。
		Uint32 triangleMaterialIndexBufferIndex_ = 0;

		/// [EN] Minimum of the UV bounding box; the compressed vertex UVs are UNORM within it, so the closest-hit shader needs it to decode them.
		/// [JP] UV のバウンディングボックスの最小値。圧縮された頂点 UV はこの範囲内の UNORM なので、最近接ヒットシェーダーがデコードに使う。
		Float texcoordMin_[2] = { 0.0f, 0.0f };

		/// [EN] Extent of the UV bounding box (Crister::TexcoordExtent, the same value the raster path uses).
		/// [JP] UV のバウンディングボックスの大きさ（Crister::TexcoordExtent。ラスタ経路と同じ値）。
		Float texcoordExtent_[2] = { 1.0f, 1.0f };
	};

	/**
	* [EN]
	* Runs the ray-traced glossy reflections. ReflectionRT.hlsl (ray
	* generation, miss and closest hit through DispatchRays) GGX-samples a
	* ReSTIR reservoir and writes a raw RGBA16F radiance texture (rgb is
	* incoming reflected radiance, a the averaged hit distance in world
	* units); a spatial reuse pass refines it together with a confidence
	* texture; and, unless DLSS Ray Reconstruction is on, ReflectionDenoiseCS.hlsl
	* runs a five-pass SVGF chain: dual-reprojection temporal accumulation
	* with moments, a spatial variance estimate for short histories, and three
	* variance-guided A-Trous iterations, one chain per view. At roughness 0
	* the GGX sample becomes an exact mirror ray. The result is left in
	* shader-resource state for DeferredLightingPS.hlsl. The structure matches
	* ShadowRenderer, extended from binary visibility to HDR RGB radiance with
	* a hit-point virtual-motion reprojection candidate. The renderer also
	* owns the per-frame instance table that RaytracingRenderer fills while
	* building the TLAS, and the three-record shader table.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングによる光沢反射を実行する。ReflectionRT.hlsl（DispatchRays
	* によるレイ生成、ミス、最近接ヒット）が GGX で ReSTIR の reservoir を
	* サンプルし、生の RGBA16F 放射輝度テクスチャ（rgb は入射する反射の
	* 放射輝度、a はワールド単位の平均ヒット距離）を書き、空間的リユースの
	* パスがそれを信頼度テクスチャと共に整える。DLSS Ray Reconstruction が
	* 無効なら、さらに ReflectionDenoiseCS.hlsl が 5 パスの SVGF チェーン
	* （モーメントを伴う二重リプロジェクションの時間方向の蓄積、履歴の短い
	* ピクセル向けの空間的な分散の推定、分散に導かれる A-Trous の 3 反復。
	* ビューごとに 1 チェーン）を実行する。roughness 0 では GGX のサンプルが
	* 厳密な鏡面反射のレイになる。結果は DeferredLightingPS.hlsl のために
	* シェーダーリソース状態で残す。構成は ShadowRenderer と同じで、二値の
	* 可視性を、ヒット点の仮想的な動きによるリプロジェクション候補を伴う HDR
	* の RGB 放射輝度へ広げたもの。RaytracingRenderer が TLAS の構築中に詰める
	* フレームごとのインスタンステーブルと、3 レコードのシェーダーテーブルも
	* 持つ。
	*/
	class ReflectionRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature to the raytracing-state cache the
		* ray pass compiles into and the pipeline-state cache the denoise
		* passes compile into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有のルートシグネチャを、レイのパスのコンパイル先となるレイトレー
		* シングステートキャッシュと、デノイズのパスのコンパイル先となる
		* パイプラインステートキャッシュへ関連付ける。
		*/
		ReflectionRenderer(RootSignature& rootSignature, RaytracingStateObject& raytracingStateObject, PipelineStateObject& pipelineStateObject);

		/**
		* [EN]
		* Destroys the renderer. GPU resources are released together with the
		* bindless heap at shutdown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レンダラーを破棄する。GPU リソースは終了時に bindless ヒープと共に
		* 解放される。
		*/
		~ReflectionRenderer() = default;

		/**
		* [EN]
		* Compiles the raytracing and denoise pipelines, creates the tuning
		* constant buffer, the instance table and the three-record shader
		* table, and allocates every texture and reservoir at width x height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイトレーシングとデノイズのパイプラインをコンパイルし、調整値の定数
		* バッファ、インスタンステーブル、3 レコードのシェーダーテーブルを作成
		* して、width x height のすべてのテクスチャと reservoir を確保する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates every texture and reservoir for a new render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて、すべてのテクスチャと reservoir を
		* 作り直す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Uploads this frame's instance table, in the same order as the TLAS
		* instance descs (entry i belongs to InstanceID() == i). Called by
		* RaytracingRenderer::Build right after it collects the instances.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今フレームのインスタンステーブルを、TLAS のインスタンス desc と同じ
		* 順序でアップロードする（要素 i が InstanceID() == i に対応する）。
		* RaytracingRenderer::Build がインスタンスを集めた直後に呼ぶ。
		*/
		void UpdateInstanceTable(const ReflectionInstanceData* data, Uint32 count);

		/**
		* [EN]
		* Swaps the history slots, uploads the tuning values with the frame
		* counter and temporal-reuse flag, records whether the pass runs and
		* whether DLSS Ray Reconstruction (read from the DLSS manager) replaces
		* the SVGF chain, and publishes every bindless index to the index
		* systems. It records no GPU work, and must run before the index
		* systems upload this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 履歴のスロットを入れ替え、フレームカウンターと時間方向の再利用フラグを
		* 入れた調整値をアップロードし、パスを実行するか、DLSS Ray
		* Reconstruction（DLSS マネージャーから読む）が SVGF チェーンの代わりに
		* なるかを記録して、すべての bindless インデックスを各インデックス
		* システムへ公開する。GPU 処理は記録せず、各インデックスシステムが
		* 今フレームのインデックスをアップロードする前に呼び出す。
		*/
		void Prepare(const ReflectionRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass for view: dispatches rays into the raw texture, runs
		* the spatial reuse and, unless DLSS Ray Reconstruction is on, the SVGF
		* chain - reprojection into scratch 0 with this frame's moments,
		* history length and depth-normal copy, FilterMoments into scratch 1,
		* A-Trous step 1 back into scratch 0, A-Trous step 2 into this frame's
		* history write slot (the feedback tap), and A-Trous step 4 into the
		* view's denoised output. When the pass is off, the texture deferred
		* lighting reads, the history length and this frame's reservoir are
		* cleared instead. The G-Buffer depth, normals and velocity must
		* already be written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view のパスを記録する。生のテクスチャへレイをディスパッチし、空間的
		* リユースを行い、DLSS Ray Reconstruction が無効なら SVGF チェーンを
		* 実行する。リプロジェクションを今フレームのモーメント、履歴長、深度
		* 法線コピーと共にスクラッチ 0 へ、FilterMoments をスクラッチ 1 へ、
		* A-Trous ステップ 1 をスクラッチ 0 へ戻し、A-Trous ステップ 2 を今
		* フレームの履歴の書き込みスロット（フィードバックタップ）へ、A-Trous
		* ステップ 4 をビューのデノイズ済み出力へ書く。パスが無効なら、代わりに
		* ディファードライティングが読むテクスチャ、履歴長、今フレームの
		* reservoir をクリアする。G-Buffer の深度、法線、速度が書き込み済みで
		* あることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ raw radiance and confidence textures
		* and, per view, the whole SVGF chain and the reservoirs, and marks
		* the new history chain for zeroing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の生の放射輝度と信頼度のテクスチャ、ビューごとの
		* SVGF チェーン一式と reservoir を作成し、新しい履歴チェーンを 0 埋めの
		* 対象にする。
		*/
		void Allocate(ID3D12Device* device);

		/**
		* [EN]
		* Frees every texture's and reservoir's bindless views and defers
		* their destruction until the GPU has finished with them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべてのテクスチャと reservoir の bindless ビューを解放し、それらの
		* 破棄は GPU が使い終えるまで遅延させる。
		*/
		void Release();

	private:
		/// [EN] History slots per view: one written this frame, one holding the previous frame.
		/// [JP] ビューごとの履歴のスロットの数。今フレームに書き込む 1 つと、前フレームを持つ 1 つ。
		static constexpr Uint32 accumulationSlotCount_ = 2;

		/// [EN] Number of views that keep their own history (editor and game).
		/// [JP] 独自の履歴を持つビューの数（エディターとゲーム）。
		static constexpr Uint32 viewCount_ = 2;

		/// [EN] Number of instances the instance table can hold. Must equal RaytracingRenderer's TLAS instance limit, so InstanceID() always stays inside the table.
		/// [JP] インスタンステーブルが保持できるインスタンスの数。InstanceID() が常にテーブルの範囲内に収まるよう、RaytracingRenderer の TLAS のインスタンス上限と等しくする。
		static constexpr Uint32 maxInstances_ = 4096;

		/// [EN] Size in bytes of one reservoir element; must match ReflectionReservoir in ReflectionReSTIR.hlsli.
		/// [JP] reservoir の要素 1 つのバイト数。ReflectionReSTIR.hlsli の ReflectionReservoir と一致させる。
		static constexpr Uint32 reservoirElementSizeInBytes_ = 64;

		/// [EN] Size of one shader-table record: the 32-byte shader identifier rounded up to the 64-byte table alignment.
		/// [JP] シェーダーテーブルのレコード 1 つのサイズ。32 バイトのシェーダー識別子を 64 バイトのテーブルアライメントへ切り上げたもの。
		static constexpr Uint32 shaderTableRecordSize_ = 64;

		/// [EN] Raytracing pipeline of ReflectionRT.hlsl.
		/// [JP] ReflectionRT.hlsl のレイトレーシングパイプライン。
		ReflectionShader reflectionShader_;

		/// [EN] Spatial reuse and SVGF compute pipelines of ReflectionDenoiseCS.hlsl.
		/// [JP] ReflectionDenoiseCS.hlsl の空間的リユースと SVGF のコンピュートパイプライン。
		ReflectionDenoiseShader denoiseShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<ConstantBuffer<ReflectionRayConstantBuffer>> tuningBuffer_;

		/// [EN] Per-frame instance table (InstanceID() to vertex, index and material data), shared with Refraction and GlobalIllumination.
		/// [JP] フレームごとのインスタンステーブル（InstanceID() から頂点、インデックス、マテリアルのデータへ）。Refraction と GlobalIllumination も共有する。
		ResourcePtr<ReadOnlyStructuredBuffer<ReflectionInstanceData>> instanceTable_;

		/// [EN] Raw RGBA16F radiance from the ray pass (rgb radiance, a averaged hit distance), refined in place by the spatial reuse. A single texture is enough, since the denoiser consumes it in the same flush.
		/// [JP] レイのパスが書き、空間的リユースがその場で整える生の RGBA16F 放射輝度（rgb が放射輝度、a が平均ヒット距離）。デノイザが同じ Flush 内で消費するため、1 枚で足りる。
		Microsoft::WRL::ComPtr<ID3D12Resource> radianceResource_;
		D3D12_RESOURCE_STATES radianceState_ = D3D12_RESOURCE_STATE_COMMON;
		Uint32 radianceUnorderedAccessViewIndex_ = 0;
		Uint32 radianceShaderResourceViewIndex_ = 0;

		/// [EN] Per-pixel 0-1 confidence written by the spatial reuse next to the radiance. The denoiser reads it to let its own temporal blend defer to the reservoir's already converged history instead of stacking a second one on top.
		/// [JP] 空間的リユースが放射輝度と並べて書く、ピクセルごとの 0 から 1 の信頼度。デノイザはこれを読み、自身の時間方向のブレンドを reservoir の既に収束した履歴に譲り、2 段目を重ねないようにする。
		Microsoft::WRL::ComPtr<ID3D12Resource> confidenceResource_;
		D3D12_RESOURCE_STATES confidenceState_ = D3D12_RESOURCE_STATE_COMMON;
		Uint32 confidenceUnorderedAccessViewIndex_ = 0;
		Uint32 confidenceShaderResourceViewIndex_ = 0;

		/// [EN] SVGF temporal history (rgb filtered radiance, a its variance), two slots per view. This is the feedback tap written by A-Trous step 2 and read by next frame's reprojection, not the final image.
		/// [JP] SVGF の時間方向の履歴（rgb はフィルタ済みの放射輝度、a はその分散）。ビューごとに 2 スロット。A-Trous ステップ 2 が書き、次フレームのリプロジェクションが読むフィードバックタップで、最終画ではない。
		Microsoft::WRL::ComPtr<ID3D12Resource> accumulatedRadianceResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES accumulatedRadianceState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 accumulatedUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 accumulatedShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] First and second luminance moments. SVGF derives variance from the temporally accumulated moments, so they carry over between frames exactly like the radiance.
		/// [JP] 輝度の 1 次と 2 次のモーメント。SVGF は時間方向に蓄積したモーメントから分散を求めるため、放射輝度と同じくフレームをまたいで引き継ぐ。
		Microsoft::WRL::ComPtr<ID3D12Resource> momentsResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES momentsState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 momentsUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 momentsShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Per-pixel count of successfully reprojected frames. It drives the max(alpha, 1 / length) blend factor and the switch to the spatial variance estimate.
		/// [JP] ピクセルごとのリプロジェクションに成功したフレーム数。max(alpha, 1 / 履歴長) のブレンド係数と、空間的な分散の推定への切り替えを決める。
		Microsoft::WRL::ComPtr<ID3D12Resource> historyLengthResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES historyLengthState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 historyLengthUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 historyLengthShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] This frame's surface packed as (view depth, depth derivative, octahedral normal). The temporal consistency test compares against the previous frame's copy, which the single-buffered G-Buffer cannot provide. It is 32-bit because SVGF compares depth differences in units of the depth derivative, and FP16 depth is coarser than that derivative at mid distances.
		/// [JP] 今フレームの面を (ビュー深度, 深度の勾配, 八面体の法線) で詰めたもの。時間方向の整合性の判定は前フレームのコピーと比べるが、単一バッファの G-Buffer では前フレームを読めない。SVGF は深度差を深度の勾配を単位として比べるが、FP16 の深度は中距離でその勾配より粗くなるため、32 ビットにしている。
		Microsoft::WRL::ComPtr<ID3D12Resource> depthNormalResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES depthNormalState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 depthNormalUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 depthNormalShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Fully filtered RGBA16F radiance that deferred lighting samples (rgb radiance, a 1 when valid, 0 for background): the output of the last A-Trous iteration, one per view. It never feeds back, which is what lets the history stop at the earlier, sharper feedback tap.
		/// [JP] ディファードライティングがサンプルする、完全にフィルタ済みの RGBA16F 放射輝度（rgb が放射輝度、a は有効なら 1、背景なら 0）。最後の A-Trous 反復の出力で、ビューごとに 1 枚。フィードバックしないため、履歴をより早くシャープなフィードバックタップで止められる。
		Microsoft::WRL::ComPtr<ID3D12Resource> denoisedResource_[viewCount_];
		D3D12_RESOURCE_STATES denoisedState_[viewCount_] = {};
		Uint32 denoisedUnorderedAccessViewIndex_[viewCount_] = {};
		Uint32 denoisedShaderResourceViewIndex_[viewCount_] = {};

		/// [EN] A-Trous scratch pair, one per view. Always fully overwritten by the pass that writes it.
		/// [JP] A-Trous 用スクラッチ 2 枚。ビューごとに 1 組。書き込むパスが必ず全画素を上書きする。
		Microsoft::WRL::ComPtr<ID3D12Resource> atrousScratchResource_[viewCount_][2];
		D3D12_RESOURCE_STATES atrousScratchState_[viewCount_][2] = {};
		Uint32 atrousScratchUnorderedAccessViewIndex_[viewCount_][2] = {};
		Uint32 atrousScratchShaderResourceViewIndex_[viewCount_][2] = {};

		/// [EN] ReSTIR reservoirs, two slots per view like the history. Ray generation reads last frame's slot (reprojected) and writes this frame's slot in the same dispatch, then the spatial reuse reads this frame's slot again; each is a screen-sized structured buffer of reservoirElementSizeInBytes_-byte elements.
		/// [JP] ReSTIR の reservoir。履歴と同じくビューごとに 2 スロット。レイ生成が同じディスパッチの中で前フレームのスロットを（再投影して）読み、今フレームのスロットへ書き、その後空間的リユースが今フレームのスロットを読み直す。それぞれ reservoirElementSizeInBytes_ バイトの要素を持つ画面サイズの構造化バッファ。
		Microsoft::WRL::ComPtr<ID3D12Resource> reservoirResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES reservoirState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 reservoirUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 reservoirShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Slot holding the previous frame's result, i.e. this frame's history. It swaps once per frame in Prepare, not in Dispatch, which runs once per view.
		/// [JP] 前フレームの結果、つまり今フレームの履歴を持つスロット。入れ替えは 1 フレームに 1 回 Prepare で行い、ビューごとに走る Dispatch では行わない。
		Uint32 historySlot_ = 0;

		/// [EN] Upload-heap shader table of three records: ray generation at 0, miss at 64, hit group at 128. Each record is a bare shader identifier with no local root arguments, built once in Create because identifiers stay valid for the state object's lifetime.
		/// [JP] 3 レコードのアップロードヒープ上のシェーダーテーブル。レイ生成が 0、ミスが 64、ヒットグループが 128。各レコードはローカルルート引数の無いシェーダー識別子だけで、識別子はステートオブジェクトの生存中は変わらないため Create で 1 度だけ作る。
		Microsoft::WRL::ComPtr<ID3D12Resource> shaderTableResource_;

		/// [EN] Non-shader-visible heap holding the CPU-side write views that the clears require: the raw radiance, the confidence, each view's denoised output, and every buffer of the history chain and every reservoir.
		/// [JP] クリアが要求する CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。生の放射輝度、信頼度、ビューごとのデノイズ済み出力、履歴チェーンのすべてのバッファとすべての reservoir の分。
		DescriptorHeap clearHeap_;

		/// [EN] Indices of the clearable resources' write views inside clearHeap_.
		/// [JP] clearHeap_ 内での、クリアするリソースの書き込み用ビューのインデックス。
		Uint32 clearRawIndex_ = 0;
		Uint32 clearConfidenceIndex_ = 0;
		Uint32 clearDenoisedIndex_[viewCount_] = {};
		Uint32 clearHistoryLengthIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearAccumulatedIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearMomentsIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearDepthNormalIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearReservoirIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Shader-visible raw write view matching each reservoir's clear view. ClearUnorderedAccessViewUint needs the same non-structured view both on the GPU side and on the CPU side (see ReservoirBuffer::Create).
		/// [JP] 各 reservoir のクリア用ビューと対になる、シェーダー可視の raw 書き込み用ビュー。ClearUnorderedAccessViewUint は GPU 側と CPU 側の両方に同じ非構造化ビューを要求する（ReservoirBuffer::Create 参照）。
		Uint32 clearReservoirGpuIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Whether the history chain and both reservoir slots have been zeroed since allocation. A new committed resource is not guaranteed to read as zero, and every history buffer feeds back into itself, so an uninitialized texel would persist (undefined FP16 bits are readily NaN). A zeroed reservoir (M_ and W_ at 0) makes the first frame treat its history as absent.
		/// [JP] 確保以降に履歴チェーンと reservoir の両スロットを 0 で埋めたか。生成直後の committed リソースが 0 で読める保証は無く、履歴のバッファはすべて自分自身へ戻るため、未初期化のテクセルは消えずに残り続ける（未定義の FP16 のビットは容易に NaN になる）。0 の reservoir（M_ と W_ が 0）なら、最初のフレームは履歴を無いものとして扱う。
		Bool historyCleared_ = false;

		/// [EN] Bindless heap that owns this pass's descriptors.
		/// [JP] このパスのディスクリプタを所有する bindless ヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Index systems that receive this pass's bindless indices in Prepare.
		/// [JP] Prepare でこのパスの bindless インデックスを受け取るインデックスシステム。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;
		UnorderedAccessIndicesSystem* unorderedAccessIndicesSystem_ = nullptr;

		/// [EN] Size of every texture, equal to the native render size.
		/// [JP] すべてのテクスチャのサイズ。ネイティブのレンダーサイズと等しい。
		Uint32 width_ = 0;
		Uint32 height_ = 0;

		/// [EN] Frame counter copied into frameIndex_ of the uploaded tuning values.
		/// [JP] アップロードする調整値の frameIndex_ へ写すフレームカウンター。
		Uint32 frameIndex_ = 0;

		/// [EN] Whether the pass traces this frame: the pass is switched on and the scene has a TLAS.
		/// [JP] 今フレームにパスをトレースするか。パスが有効で、かつシーンに TLAS がある場合に true。
		Bool enabled_ = false;

		/// [EN] Whether DLSS Ray Reconstruction denoises the frame this frame, replacing the SVGF chain.
		/// [JP] 今フレームに DLSS Ray Reconstruction がフレームをデノイズし、SVGF チェーンの代わりになるか。
		Bool useDlssRayReconstruction_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool stateObjectMissingLogged_ = false;
	};
}
