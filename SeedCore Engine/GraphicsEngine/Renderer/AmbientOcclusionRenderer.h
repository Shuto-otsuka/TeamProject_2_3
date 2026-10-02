#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionDenoiseShader.h>
#include <GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionShader.h>
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
	* Tuning values of the ambient occlusion pass. Mirrors
	* AmbientOcclusionRayConstantBuffer in
	* Raytracing/AmbientOcclusion/AmbientOcclusion.hlsli, which both
	* AmbientOcclusionRT.hlsl and DeferredLightingPS.hlsl read through
	* constant_indices.ambient_occlusion_index_, so the layout must match the
	* HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アンビエントオクルージョンのパスの調整値。
	* Raytracing/AmbientOcclusion/AmbientOcclusion.hlsli の
	* AmbientOcclusionRayConstantBuffer と対応する。AmbientOcclusionRT.hlsl と
	* DeferredLightingPS.hlsl の両方が constant_indices.ambient_occlusion_index_
	* 経由で読むため、レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct AmbientOcclusionRayConstantBuffer
	{
		/// [EN] World-space occlusion radius; occluders farther away than this do not darken the surface.
		/// [JP] ワールド空間の遮蔽半径。これより遠い遮蔽物は表面を暗くしない。
		Float rayLength_ = 1.0f;

		/// [EN] Offset along the surface normal applied to each ray origin, so a ray does not hit the surface it leaves.
		/// [JP] 各レイの原点に法線方向へ加えるオフセット。出発した面にレイがヒットしないようにする。
		Float normalBias_ = 0.01f;

		/// [EN] Contrast exponent applied as pow(ao, power_) in DeferredLightingPS.hlsl: 1 leaves it as is, above 1 darkens it.
		/// [JP] DeferredLightingPS.hlsl で pow(ao, power_) として適用するコントラストの指数。1 ならそのまま、1 より大きいと暗く強くなる。
		Float power_ = 1.0f;

		/// [EN] Frame counter written by AmbientOcclusionRenderer::Prepare, not by the editor UI. It seeds the per-pixel random numbers, so the random ray directions change every frame.
		/// [JP] エディターの UI ではなく AmbientOcclusionRenderer::Prepare が書き込むフレームカウンター。ピクセルごとの乱数の種になり、ランダムなレイの方向を毎フレーム変える。
		Uint32 frameIndex_ = 0;

		/**
		* [EN]
		* Reads or writes the tuning values through archive. The frame counter
		* is written by the renderer every frame and is not serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値を読み書きする。フレームカウンターはレンダラーが
		* 毎フレーム書き込むため、シリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("rayLength", rayLength_);
			archive.TryField("normalBias", normalBias_);
			archive.TryField("power", power_);
		}
	};

	/**
	* [EN]
	* Runs the ray-traced ambient occlusion: AmbientOcclusionRT.hlsl writes a
	* raw, noisy single-channel openness texture, and
	* AmbientOcclusionDenoiseCS.hlsl denoises it by reprojecting and blending
	* against an accumulation texture that alternates between two slots per
	* view. The result is left in shader-resource state for
	* DeferredLightingPS.hlsl. When DLSS Ray Reconstruction is on, it denoises
	* the whole frame itself, so the raw texture is read directly and the
	* denoise pass is skipped. When the pass is off, the scene has no TLAS or
	* a pipeline is missing, the texture deferred lighting reads is cleared to
	* 1.0 (fully open) instead. The structure matches ShadowRenderer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングによるアンビエントオクルージョンを実行する。
	* AmbientOcclusionRT.hlsl が生のノイズを含む 1 チャンネルの開放度
	* テクスチャを書き、AmbientOcclusionDenoiseCS.hlsl がビューごとに 2 つの
	* スロットを交互に使う蓄積テクスチャとのリプロジェクションとブレンドで
	* それをデノイズする。結果は DeferredLightingPS.hlsl のためにシェーダー
	* リソース状態で残す。DLSS Ray Reconstruction が有効な間はそれがフレーム
	* 全体をデノイズするため、生のテクスチャを直接読ませ、デノイズパスは
	* 省く。パスが無効、シーンに TLAS が無い、またはパイプラインが無い場合は、
	* ディファードライティングが読むテクスチャを 1.0（完全に開放）でクリア
	* する。構成は ShadowRenderer と同じ。
	*/
	class AmbientOcclusionRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and pipeline-state cache that the
		* occlusion and denoise shaders compile into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 遮蔽とデノイズのシェーダーのコンパイル先となる、共有のルート
		* シグネチャとパイプラインステートキャッシュを関連付ける。
		*/
		AmbientOcclusionRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

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
		~AmbientOcclusionRenderer() = default;

		/**
		* [EN]
		* Compiles the shaders, creates the tuning constant buffer, and
		* allocates the raw and accumulation textures at width x height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、調整値の定数バッファを作成して、
		* width x height の生のテクスチャと蓄積テクスチャを確保する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates the raw and accumulation textures for a new render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて、生のテクスチャと蓄積テクスチャを
		* 作り直す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Swaps the accumulation slots, uploads the tuning values with the
		* frame counter, records whether the pass runs and whether DLSS Ray
		* Reconstruction (read from the DLSS manager) replaces the denoiser,
		* and publishes every bindless index to the index systems. It records
		* no GPU work, and must run before the index systems upload this
		* frame's indices - that is, before the G-Buffer exists.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 蓄積のスロットを入れ替え、フレームカウンターを入れた調整値を
		* アップロードし、パスを実行するか、DLSS Ray Reconstruction（DLSS
		* マネージャーから読む）がデノイザの代わりになるかを記録して、すべての
		* bindless インデックスを各インデックスシステムへ公開する。GPU 処理は
		* 記録せず、各インデックスシステムが今フレームのインデックスを
		* アップロードする前、つまり G-Buffer ができる前に呼び出す。
		*/
		void Prepare(const AmbientOcclusionRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass for view: traces into the raw texture and, unless
		* DLSS Ray Reconstruction is on, denoises it into this frame's
		* accumulation slot; or clears the texture deferred lighting reads to
		* 1.0 when the pass is off. That texture is left in shader-resource
		* state. The G-Buffer depth, normals and velocity must already be
		* written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view のパスを記録する。生のテクスチャへトレースし、DLSS Ray
		* Reconstruction が無効ならそれを今フレームの蓄積スロットへデノイズする。
		* パスが無効なら、ディファードライティングが読むテクスチャを 1.0 で
		* クリアする。そのテクスチャはシェーダーリソース状態で終える。
		* G-Buffer の深度、法線、速度が書き込み済みであることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ raw texture and every accumulation
		* texture, each with bindless write and read views and a clear view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の生のテクスチャとすべての蓄積テクスチャを、それぞれ
		* bindless の書き込み用・読み取り用ビューとクリア用ビュー付きで作成する。
		*/
		void Allocate(ID3D12Device* device);

		/**
		* [EN]
		* Frees every texture's bindless views and defers the textures'
		* destruction until the GPU has finished with them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべてのテクスチャの bindless ビューを解放し、テクスチャ自体の破棄は
		* GPU が使い終えるまで遅延させる。
		*/
		void Release();

	private:
		/// [EN] Accumulation slots per view: one written this frame, one holding the previous frame as history.
		/// [JP] ビューごとの蓄積スロットの数。今フレームに書き込む 1 つと、前フレームを履歴として持つ 1 つ。
		static constexpr Uint32 accumulationSlotCount_ = 2;

		/// [EN] Number of views that keep their own accumulation (editor and game).
		/// [JP] 独自の蓄積を持つビューの数（エディターとゲーム）。
		static constexpr Uint32 viewCount_ = 2;

		/// [EN] Compute shader and pipeline of AmbientOcclusionRT.hlsl.
		/// [JP] AmbientOcclusionRT.hlsl のコンピュートシェーダーとパイプライン。
		AmbientOcclusionShader ambientOcclusionShader_;

		/// [EN] Compute shader and pipeline of AmbientOcclusionDenoiseCS.hlsl.
		/// [JP] AmbientOcclusionDenoiseCS.hlsl のコンピュートシェーダーとパイプライン。
		AmbientOcclusionDenoiseShader denoiseShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<StaticConstantBuffer<AmbientOcclusionRayConstantBuffer>> tuningBuffer_;

		/// [EN] Raw, noisy single-channel openness written by AmbientOcclusionRT.hlsl. A single texture is enough, since the denoiser consumes it in the same flush.
		/// [JP] AmbientOcclusionRT.hlsl が書く、生のノイズを含む 1 チャンネルの開放度。デノイザが同じ Flush 内で消費するため、1 枚で足りる。
		Microsoft::WRL::ComPtr<ID3D12Resource> rawOpennessResource_;

		/// [EN] Resource state the raw texture is currently in.
		/// [JP] 生のテクスチャの現在のリソース状態。
		D3D12_RESOURCE_STATES rawOpennessState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless indices of the raw texture's write and read views.
		/// [JP] 生のテクスチャの書き込み用と読み取り用ビューの bindless インデックス。
		Uint32 rawOpennessUnorderedAccessViewIndex_ = 0;
		Uint32 rawOpennessShaderResourceViewIndex_ = 0;

		/// [EN] Denoised openness accumulated over time, two slots per view that swap roles every frame.
		/// [JP] 時間方向に蓄積したデノイズ済みの開放度。ビューごとに 2 つのスロットを持ち、毎フレーム役割を入れ替える。
		Microsoft::WRL::ComPtr<ID3D12Resource> accumulatedOpennessResource_[viewCount_][accumulationSlotCount_];

		/// [EN] Resource state each accumulation texture is currently in.
		/// [JP] 各蓄積テクスチャの現在のリソース状態。
		D3D12_RESOURCE_STATES accumulatedOpennessState_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Bindless indices of each accumulation texture's write and read views.
		/// [JP] 各蓄積テクスチャの書き込み用と読み取り用ビューの bindless インデックス。
		Uint32 accumulatedUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 accumulatedShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Slot holding the previous frame's result, i.e. this frame's history. It swaps once per frame in Prepare, not in Dispatch, which runs once per view.
		/// [JP] 前フレームの結果、つまり今フレームの履歴を持つスロット。入れ替えは 1 フレームに 1 回 Prepare で行い、ビューごとに走る Dispatch では行わない。
		Uint32 historySlot_ = 0;

		/// [EN] Non-shader-visible heap holding the CPU-side write views that ClearUnorderedAccessViewFloat requires: one for the raw texture and one per accumulation texture.
		/// [JP] ClearUnorderedAccessViewFloat が要求する CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。生のテクスチャに 1 つ、蓄積テクスチャごとに 1 つ。
		DescriptorHeap clearHeap_;

		/// [EN] Index of the raw texture's write view inside clearHeap_.
		/// [JP] clearHeap_ 内での生のテクスチャの書き込み用ビューのインデックス。
		Uint32 clearRawIndex_ = 0;

		/// [EN] Indices of the accumulation textures' write views inside clearHeap_.
		/// [JP] clearHeap_ 内での蓄積テクスチャの書き込み用ビューのインデックス。
		Uint32 clearAccumulatedIndex_[viewCount_][accumulationSlotCount_] = {};

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

		/// [EN] Whether DLSS Ray Reconstruction denoises the frame this frame, replacing this pass's denoiser.
		/// [JP] 今フレームに DLSS Ray Reconstruction がフレームをデノイズし、このパスのデノイザの代わりになるか。
		Bool useDlssRayReconstruction_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool pipelineStateMissingLogged_ = false;
	};
}
