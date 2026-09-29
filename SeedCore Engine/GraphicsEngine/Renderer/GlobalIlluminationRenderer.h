#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationDenoiseShader.h>
#include <GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationShader.h>
#include <GraphicsEngine/Raytracing/RaytracingDispatch.h>

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
	* Tuning values of the global illumination pass. Mirrors
	* GlobalIlluminationRayConstantBuffer in
	* Raytracing/GlobalIllumination/GlobalIllumination.hlsli, which both
	* GlobalIlluminationRT.hlsl and DeferredLightingPS.hlsl read through
	* constant_indices.global_illumination_index_, so the layout must match
	* the HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グローバルイルミネーションのパスの調整値。
	* Raytracing/GlobalIllumination/GlobalIllumination.hlsli の
	* GlobalIlluminationRayConstantBuffer と対応する。
	* GlobalIlluminationRT.hlsl と DeferredLightingPS.hlsl の両方が
	* constant_indices.global_illumination_index_ 経由で読むため、レイアウトは
	* HLSL 側とバイト単位で一致させる。
	*/
	struct GlobalIlluminationRayConstantBuffer
	{
		/// [EN] How far an indirect ray travels before it counts as reaching the sky. Too short and interiors lose their bounce light; too long and every ray pays for a full-scene traversal.
		/// [JP] 間接レイが空に届いたとみなすまでの距離。短すぎると室内のバウンス光が消え、長すぎるとすべてのレイがシーン全体の走査コストを払う。
		Float rayTMax_ = 2000.0f;

		/// [EN] Offset along the surface normal applied to each ray origin, so a ray does not hit the surface it leaves.
		/// [JP] 各レイの原点に法線方向へ加えるオフセット。出発した面にレイがヒットしないようにする。
		Float normalBias_ = 0.05f;

		/// [EN] Overall indirect-light intensity, applied in DeferredLightingPS.hlsl.
		/// [JP] 間接光全体の強さ。DeferredLightingPS.hlsl で適用する。
		Float intensity_ = 1.0f;

		/// [EN] Frame counter written by GlobalIlluminationRenderer::Prepare, not by the editor UI. It rotates the hemisphere sample, so the one-sample-per-pixel noise averages out over time instead of forming a fixed pattern.
		/// [JP] エディターの UI ではなく GlobalIlluminationRenderer::Prepare が書き込むフレームカウンター。半球のサンプルを回し、1 ピクセル 1 サンプルのノイズが固定の模様にならず時間方向に平均化されるようにする。
		Uint32 frameIndex_ = 0;

		/// [EN] 1 while the reservoir reuses its history across frames; Prepare writes 0 when DLSS Ray Reconstruction does the temporal denoising instead.
		/// [JP] reservoir がフレームをまたいで履歴を再利用する間 1。DLSS Ray Reconstruction が時間方向のデノイズを担うときは Prepare が 0 を書く。
		Uint32 temporalReuseEnabled_ = 1;

		/// [EN] Pads the structure to two full 16-byte registers.
		/// [JP] 構造体を 16 バイトのレジスタ 2 つ分に揃える詰め物。
		Vector3 globalIlluminationRayPadding_ = { 0.0f, 0.0f, 0.0f };

		/**
		* [EN]
		* Reads or writes the tuning values through archive. The values
		* written by the renderer every frame and the padding are not
		* serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値を読み書きする。レンダラーが毎フレーム書き込む
		* 値と詰め物はシリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("rayTMax", rayTMax_);
			archive.TryField("normalBias", normalBias_);
			archive.TryField("intensity", intensity_);
		}
	};

	/// [EN] The HLSL side is two 16-byte rows (8 scalars); the size is asserted here because nothing checks the layout at runtime.
	/// [JP] HLSL 側は 16 バイトの行が 2 行（8 スカラー）。実行時にはレイアウトを検証できないため、ここでサイズを静的に検証する。
	static_assert(sizeof(GlobalIlluminationRayConstantBuffer) == 8 * sizeof(Float), "GlobalIlluminationRayConstantBuffer が GlobalIllumination.hlsli とバイト単位で一致していません");

	/**
	* [EN]
	* Runs the ray-traced one-bounce diffuse global illumination.
	* GlobalIlluminationRT.hlsl (ray generation, miss and closest hit through
	* DispatchRays) resamples a ReSTIR reservoir and writes a raw RGBA16F
	* radiance texture (rgb is incoming indirect radiance, a is 1 when valid);
	* a spatial reuse pass refines it together with a confidence texture; and,
	* unless DLSS Ray Reconstruction is on, GlobalIlluminationDenoiseCS.hlsl
	* blends it over time and runs three A-Trous iterations into an
	* accumulation texture that alternates between two slots per view. The
	* result is left in shader-resource state for DeferredLightingPS.hlsl,
	* which multiplies it by the receiving surface's albedo. The structure
	* extends AmbientOcclusionRenderer from a scalar signal to RGB. The bounce
	* surface's geometry and material come from Reflection's instance table
	* rather than a second copy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングによる 1 バウンスの拡散グローバルイルミネーションを
	* 実行する。GlobalIlluminationRT.hlsl（DispatchRays によるレイ生成、
	* ミス、最近接ヒット）が ReSTIR の reservoir をリサンプルして生の RGBA16F
	* 放射輝度テクスチャ（rgb は入射する間接放射輝度、a は有効なら 1）を書き、
	* 空間的リユースのパスがそれを信頼度テクスチャと共に整える。DLSS Ray
	* Reconstruction が無効なら、さらに GlobalIlluminationDenoiseCS.hlsl が
	* 時間方向にブレンドし、A-Trous を 3 反復して、ビューごとに 2 つの
	* スロットを交互に使う蓄積テクスチャへ書く。結果は
	* DeferredLightingPS.hlsl のためにシェーダーリソース状態で残し、受け側の
	* 面のアルベドを掛けて使う。構成は AmbientOcclusionRenderer をスカラーから
	* RGB へ広げたもの。バウンス先の面のジオメトリとマテリアルは、2 つ目の
	* コピーを作らず Reflection のインスタンステーブルを使う。
	*/
	class GlobalIlluminationRenderer
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
		GlobalIlluminationRenderer(RootSignature& rootSignature, RaytracingStateObject& raytracingStateObject, PipelineStateObject& pipelineStateObject);

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
		~GlobalIlluminationRenderer() = default;

		/**
		* [EN]
		* Compiles the raytracing and denoise pipelines, creates the tuning
		* constant buffer and the three-record shader table, and allocates
		* every texture and reservoir at width x height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイトレーシングとデノイズのパイプラインをコンパイルし、調整値の
		* 定数バッファと 3 レコードのシェーダーテーブルを作成して、width x height
		* のすべてのテクスチャと reservoir を確保する。
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
		* Swaps the history slots, uploads the tuning values with the frame
		* counter and temporal-reuse flag, records whether the pass runs and
		* whether DLSS Ray Reconstruction (read from the DLSS manager) replaces
		* the denoiser, and publishes every bindless index to the index
		* systems. It records no GPU work, and must run before the index
		* systems upload this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 履歴のスロットを入れ替え、フレームカウンターと時間方向の再利用フラグを
		* 入れた調整値をアップロードし、パスを実行するか、DLSS Ray
		* Reconstruction（DLSS マネージャーから読む）がデノイザの代わりになるかを
		* 記録して、すべての bindless インデックスを各インデックスシステムへ
		* 公開する。GPU 処理は記録せず、各インデックスシステムが今フレームの
		* インデックスをアップロードする前に呼び出す。
		*/
		void Prepare(const GlobalIlluminationRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass for view: dispatches rays into the raw texture, runs
		* the spatial reuse and, unless DLSS Ray Reconstruction is on, the
		* denoise chain into this frame's accumulation slot. When the pass is
		* off, the texture deferred lighting reads and this frame's reservoir
		* are cleared instead. The G-Buffer depth, normals and velocity must
		* already be written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view のパスを記録する。生のテクスチャへレイをディスパッチし、空間的
		* リユースを行い、DLSS Ray Reconstruction が無効ならデノイズの
		* チェーンで今フレームの蓄積スロットへ書く。パスが無効なら、代わりに
		* ディファードライティングが読むテクスチャと今フレームの reservoir を
		* クリアする。G-Buffer の深度、法線、速度が書き込み済みであることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ raw radiance and confidence textures,
		* and per view the accumulation textures, reservoirs and A-Trous
		* scratch pair, and marks the new reservoirs for zeroing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の生の放射輝度と信頼度のテクスチャ、ビューごとの
		* 蓄積テクスチャ、reservoir、A-Trous のスクラッチ 2 枚を作成し、新しい
		* reservoir を 0 埋めの対象にする。
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

		/// [EN] Size in bytes of one reservoir element; must match GlobalIlluminationReservoir in GlobalIllumination.hlsli.
		/// [JP] reservoir の要素 1 つのバイト数。GlobalIllumination.hlsli の GlobalIlluminationReservoir と一致させる。
		static constexpr Uint32 reservoirElementSizeInBytes_ = 64;

		/// [EN] Size of one shader-table record: the 32-byte shader identifier rounded up to the 64-byte table alignment.
		/// [JP] シェーダーテーブルのレコード 1 つのサイズ。32 バイトのシェーダー識別子を 64 バイトのテーブルアライメントへ切り上げたもの。
		static constexpr Uint32 shaderTableRecordSize_ = 64;

		/// [EN] Raytracing pipeline of GlobalIlluminationRT.hlsl.
		/// [JP] GlobalIlluminationRT.hlsl のレイトレーシングパイプライン。
		GlobalIlluminationShader globalIlluminationShader_;

		/// [EN] Spatial reuse, temporal denoise and A-Trous compute pipelines of GlobalIlluminationDenoiseCS.hlsl.
		/// [JP] GlobalIlluminationDenoiseCS.hlsl の空間的リユース、時間方向のデノイズ、A-Trous のコンピュートパイプライン。
		GlobalIlluminationDenoiseShader denoiseShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<ConstantBuffer<GlobalIlluminationRayConstantBuffer>> tuningBuffer_;

		/// [EN] Raw RGBA16F radiance from the ray pass, refined in place by the spatial reuse. A single texture is enough, since the denoiser consumes it in the same flush.
		/// [JP] レイのパスが書き、空間的リユースがその場で整える生の RGBA16F 放射輝度。デノイザが同じ Flush 内で消費するため、1 枚で足りる。
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

		/// [EN] Denoised radiance accumulated over time, two slots per view that swap roles every frame.
		/// [JP] 時間方向に蓄積したデノイズ済みの放射輝度。ビューごとに 2 つのスロットを持ち、毎フレーム役割を入れ替える。
		Microsoft::WRL::ComPtr<ID3D12Resource> accumulatedRadianceResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES accumulatedRadianceState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 accumulatedUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 accumulatedShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] A-Trous scratch pair, one per view. Pure intermediates, fully overwritten by every pass every frame, so they need no history slots.
		/// [JP] A-Trous 用スクラッチ 2 枚。ビューごとに 1 組。毎フレーム各パスが全画素を上書きする純粋な中間バッファなので、履歴のスロットは要らない。
		Microsoft::WRL::ComPtr<ID3D12Resource> atrousScratchResource_[viewCount_][2];
		D3D12_RESOURCE_STATES atrousScratchState_[viewCount_][2] = {};
		Uint32 atrousScratchUnorderedAccessViewIndex_[viewCount_][2] = {};
		Uint32 atrousScratchShaderResourceViewIndex_[viewCount_][2] = {};

		/// [EN] ReSTIR reservoirs, two slots per view like the accumulation. Ray generation reads last frame's slot (reprojected) and writes this frame's slot in the same dispatch; each is a screen-sized structured buffer of reservoirElementSizeInBytes_-byte elements.
		/// [JP] ReSTIR の reservoir。蓄積と同じくビューごとに 2 スロット。レイ生成が同じディスパッチの中で前フレームのスロットを（再投影して）読み、今フレームのスロットへ書く。それぞれ reservoirElementSizeInBytes_ バイトの要素を持つ画面サイズの構造化バッファ。
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

		/// [EN] Non-shader-visible heap holding the CPU-side write views that the clears require: the raw radiance, the confidence, and each accumulation texture and reservoir.
		/// [JP] クリアが要求する CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。生の放射輝度、信頼度、各蓄積テクスチャと reservoir の分。
		DescriptorHeap clearHeap_;

		/// [EN] Indices of the clearable resources' write views inside clearHeap_.
		/// [JP] clearHeap_ 内での、クリアするリソースの書き込み用ビューのインデックス。
		Uint32 clearRawIndex_ = 0;
		Uint32 clearConfidenceIndex_ = 0;
		Uint32 clearAccumulatedIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearReservoirIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Shader-visible raw write view matching each reservoir's clear view. ClearUnorderedAccessViewUint needs the same non-structured view both on the GPU side and on the CPU side (see ReservoirBuffer::Create).
		/// [JP] 各 reservoir のクリア用ビューと対になる、シェーダー可視の raw 書き込み用ビュー。ClearUnorderedAccessViewUint は GPU 側と CPU 側の両方に同じ非構造化ビューを要求する（ReservoirBuffer::Create 参照）。
		Uint32 clearReservoirGpuIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Whether both reservoir slots have been zeroed since they were allocated. With M_ and W_ at 0, the first frame's temporal combine treats the history as absent instead of resampling uninitialized data.
		/// [JP] 確保以降に reservoir の両スロットを 0 で埋めたか。M_ と W_ が 0 なら、最初のフレームの時間方向の結合は履歴を無いものとして扱い、未初期化のデータをリサンプルしない。
		Bool reservoirCleared_ = false;

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

		/// [EN] Whether DLSS Ray Reconstruction denoises the frame this frame, replacing this pass's denoise chain.
		/// [JP] 今フレームに DLSS Ray Reconstruction がフレームをデノイズし、このパスのデノイズチェーンの代わりになるか。
		Bool useDlssRayReconstruction_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool stateObjectMissingLogged_ = false;
	};
}
