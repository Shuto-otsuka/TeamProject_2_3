#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/RaytracingDispatch.h>
#include <GraphicsEngine/Raytracing/Shadow/ShadowDenoiseShader.h>
#include <GraphicsEngine/Raytracing/Shadow/ShadowShader.h>

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
	* Which denoiser cleans up the traced shadows. ShadowRenderer::Prepare
	* reads the DLSS manager's Ray Reconstruction toggle and writes the
	* result into ShadowRayConstantBuffer::denoiseMode_ every frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* トレースした影をどのデノイザで整えるか。ShadowRenderer::Prepare が
	* 毎フレーム DLSS マネージャーの Ray Reconstruction の設定を読み、
	* その結果を ShadowRayConstantBuffer::denoiseMode_ へ書き込む。
	*/
	enum class ShadowDenoiseMode : Uint32
	{
		/// [EN] This pass's own SVGF chain with temporal reprojection.
		/// [JP] 時間方向のリプロジェクションを伴う、このパス自身の SVGF チェーン。
		Temporal = 0,

		/// [EN] DLSS Ray Reconstruction, which denoises the whole composited frame; the SVGF chain is skipped and the raw visibility is read directly, since denoising twice would fight it.
		/// [JP] 合成したフレーム全体をデノイズする DLSS Ray Reconstruction。二重にデノイズすると衝突するため、SVGF チェーンは省き、生の可視性を直接読む。
		DlssRR = 1,
	};

	/**
	* [EN]
	* Tuning values of the shadow pass. Mirrors ShadowRayConstantBuffer in
	* Raytracing/Shadow/Shadow.hlsli, which both ShadowRT.hlsl and
	* DeferredLightingPS.hlsl read through constant_indices.shadow_index_, so
	* the layout must match the HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 影のパスの調整値。Raytracing/Shadow/Shadow.hlsli の
	* ShadowRayConstantBuffer と対応する。ShadowRT.hlsl と
	* DeferredLightingPS.hlsl の両方が constant_indices.shadow_index_ 経由で
	* 読むため、レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct ShadowRayConstantBuffer
	{
		/// [EN] Maximum length of a shadow ray; occluders beyond it cast no shadow.
		/// [JP] シャドウレイの最大長。これより遠い遮蔽物は影を落とさない。
		Float rayTMax_ = 1000.0f;

		/// [EN] Offset along the surface normal applied to each ray origin, so a ray does not hit the surface it leaves.
		/// [JP] 各レイの原点に法線方向へ加えるオフセット。出発した面にレイがヒットしないようにする。
		Float normalBias_ = 0.01f;

		/// [EN] Shadow darkness: 0 ignores the traced visibility (always lit), 1 applies it as is, 2 also removes the indirect light inside sun-facing shadows (a fully black umbra).
		/// [JP] 影の濃さ。0 はトレースした可視性を無視（常に照射）、1 はそのまま適用、2 は太陽を向いた面の影の中の間接光も消す（本影が真っ黒）。
		Float shadowStrength_ = 1.0f;

		/// [EN] Angular radius (radians) of the directional light's disc, i.e. the size of the soft-shadow cone; 0 gives hard shadows.
		/// [JP] ディレクショナルライトの円盤の角半径（ラジアン）。ソフトシャドウのコーンの大きさで、0 なら硬い影。
		Float sunAngularRadius_ = 0.02f;

		/// [EN] World-space radius used to soften point and spot light shadows.
		/// [JP] ポイントライトとスポットライトの影を柔らかくするワールド空間の半径。
		Float punctualLightRadius_ = 0.1f;

		/// [EN] Frame counter written by ShadowRenderer::Prepare, not by the editor UI. It seeds the per-pixel random numbers, so the random ray directions change every frame.
		/// [JP] エディターの UI ではなく ShadowRenderer::Prepare が書き込むフレームカウンター。ピクセルごとの乱数の種になり、ランダムなレイの方向を毎フレーム変える。
		Uint32 frameIndex_ = 0;

		/// [EN] Active ShadowDenoiseMode. Written by ShadowRenderer::Prepare every frame.
		/// [JP] 有効な ShadowDenoiseMode。ShadowRenderer::Prepare が毎フレーム書き込む。
		Uint32 denoiseMode_ = static_cast<Uint32>(ShadowDenoiseMode::Temporal);

		/// [EN] Pads the last row to a full 16-byte register.
		/// [JP] 最後の行を 16 バイトのレジスタ 1 つ分に揃える詰め物。
		Float shadowRayPadding_ = 0.0f;

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
			archive.TryField("rayTMax", rayTMax_);
			archive.TryField("normalBias", normalBias_);
			archive.TryField("shadowStrength", shadowStrength_);
			archive.TryField("sunAngularRadius", sunAngularRadius_);
			archive.TryField("punctualLightRadius", punctualLightRadius_);
			archive.TryField("denoiseMode", denoiseMode_);
		}
	};

	/**
	* [EN]
	* Runs the ray-traced shadows. ShadowRT.hlsl writes a raw, noisy texture
	* (r is directional visibility, gba is the full BRDF RGB radiance of the
	* punctual light picked by ReSTIR), then ShadowDenoiseCS.hlsl runs a
	* five-pass SVGF chain over it: temporal reprojection with moment
	* accumulation, a spatial variance estimate for pixels with a short
	* history, and three variance-guided A-Trous wavelet iterations. Each pass
	* filters the directional and punctual signals together over one shared
	* geometry chain. The result is left in shader-resource state for
	* DeferredLightingPS.hlsl. With DLSS Ray Reconstruction on, the chain is
	* skipped and the raw texture is read directly. When the pass is off, the
	* scene has no TLAS or a pipeline is missing, the textures deferred
	* lighting reads are cleared to fully lit and no punctual light instead.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングによる影を実行する。ShadowRT.hlsl が生のノイズを含む
	* テクスチャ（r はディレクショナルの可視性、gba は ReSTIR で選んだ
	* パンクチュアルライト 1 灯の完全な BRDF の RGB 放射輝度）を書き、
	* ShadowDenoiseCS.hlsl がそれに 5 パスの SVGF チェーンをかける。
	* モーメントを蓄積する時間方向のリプロジェクション、履歴の短い
	* ピクセル向けの空間的な分散の推定、分散に導かれる A-Trous
	* ウェーブレットの 3 反復である。各パスはディレクショナルと
	* パンクチュアルの信号を、共有する 1 本の幾何チェーンの上でまとめて
	* フィルタする。結果は DeferredLightingPS.hlsl のためにシェーダー
	* リソース状態で残す。DLSS Ray Reconstruction が有効な間はチェーンを
	* 省き、生のテクスチャを直接読ませる。パスが無効、シーンに TLAS が無い、
	* またはパイプラインが無い場合は、ディファードライティングが読む
	* テクスチャを「完全に照射」と「パンクチュアルライトなし」でクリアする。
	*/
	class ShadowRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and pipeline-state cache that the
		* shadow and denoise shaders compile into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 影とデノイズのシェーダーのコンパイル先となる、共有のルート
		* シグネチャとパイプラインステートキャッシュを関連付ける。
		*/
		ShadowRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

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
		~ShadowRenderer() = default;

		/**
		* [EN]
		* Compiles the shaders, creates the tuning constant buffer, and
		* allocates the raw texture and every SVGF buffer at width x height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、調整値の定数バッファを作成して、
		* width x height の生のテクスチャと SVGF のすべてのバッファを確保する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates the raw texture and every SVGF buffer for a new render
		* size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて、生のテクスチャと SVGF のすべての
		* バッファを作り直す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Swaps the history slots, uploads the tuning values with the frame
		* counter and denoise mode, records whether the pass runs and whether
		* DLSS Ray Reconstruction (read from the DLSS manager) replaces the
		* SVGF chain, and publishes every
		* bindless index - raw, history, write and the final textures deferred
		* lighting samples - to the index systems. It records no GPU work, and
		* must run before the index systems upload this frame's indices, that
		* is, before the G-Buffer exists.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 履歴のスロットを入れ替え、フレームカウンターとデノイズモードを入れた
		* 調整値をアップロードし、パスを実行するか、DLSS Ray Reconstruction
		* （DLSS マネージャーから読む）が SVGF チェーンの代わりになるかを記録
		* して、すべての bindless
		* インデックス（生、履歴、書き込み先、ディファードライティングが
		* サンプルする最終テクスチャ）を各インデックスシステムへ公開する。
		* GPU 処理は記録せず、各インデックスシステムが今フレームのインデックスを
		* アップロードする前、つまり G-Buffer ができる前に呼び出す。
		*/
		void Prepare(const ShadowRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass for view: traces into the raw texture and, unless
		* DLSS Ray Reconstruction is on, runs the SVGF chain - reprojection
		* into scratch 0, FilterMoments into scratch 1, A-Trous step 1 back
		* into scratch 0, A-Trous step 2 into this frame's history write slot
		* (the feedback tap), and A-Trous step 4 into the view's denoised
		* output. When the pass is off, the textures deferred lighting reads
		* are cleared instead and the history length is reset to 0. The
		* G-Buffer depth, normals and velocity must already be written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view のパスを記録する。生のテクスチャへトレースし、DLSS Ray
		* Reconstruction が無効なら SVGF チェーンを実行する。リプロジェクションを
		* スクラッチ 0 へ、FilterMoments をスクラッチ 1 へ、A-Trous ステップ 1 を
		* スクラッチ 0 へ戻し、A-Trous ステップ 2 を今フレームの履歴の書き込み
		* スロット（フィードバックタップ）へ、A-Trous ステップ 4 をビューの
		* デノイズ済み出力へ書く。パスが無効なら、代わりにディファード
		* ライティングが読むテクスチャをクリアし、履歴長を 0 に戻す。G-Buffer の
		* 深度、法線、速度が書き込み済みであることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ raw texture and, per view, the whole
		* SVGF chain for both signals: each signal's history, moments, A-Trous
		* scratch pair and denoised output, plus the one history length and
		* packed depth-normal copy the two signals share.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の生のテクスチャと、ビューごとに両信号の SVGF
		* チェーン一式を作成する。信号ごとの履歴、モーメント、A-Trous の
		* スクラッチ 2 枚、デノイズ済み出力と、両信号で共有する履歴長と深度・
		* 法線のパック済みコピー 1 組。
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
		/// [EN] History slots per view: one written this frame, one holding the previous frame.
		/// [JP] ビューごとの履歴のスロットの数。今フレームに書き込む 1 つと、前フレームを持つ 1 つ。
		static constexpr Uint32 accumulationSlotCount_ = 2;

		/// [EN] Number of views that keep their own history (editor and game).
		/// [JP] 独自の履歴を持つビューの数（エディターとゲーム）。
		static constexpr Uint32 viewCount_ = 2;

		/// [EN] Compute shader and pipeline of ShadowRT.hlsl.
		/// [JP] ShadowRT.hlsl のコンピュートシェーダーとパイプライン。
		ShadowShader shadowShader_;

		/// [EN] Compute shaders and pipelines of the SVGF passes in ShadowDenoiseCS.hlsl.
		/// [JP] ShadowDenoiseCS.hlsl の SVGF 各パスのコンピュートシェーダーとパイプライン。
		ShadowDenoiseShader denoiseShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<StaticConstantBuffer<ShadowRayConstantBuffer>> tuningBuffer_;

		/// [EN] Raw, noisy output of ShadowRT.hlsl: r is directional visibility, gba the visibility-weighted BRDF RGB radiance of the punctual light picked by ReSTIR. A single texture is enough, since the denoiser consumes it in the same flush.
		/// [JP] ShadowRT.hlsl の生のノイズを含む出力。r はディレクショナルの可視性、gba は ReSTIR で選んだパンクチュアルライトの、可視性を掛けた BRDF の RGB 放射輝度。デノイザが同じ Flush 内で消費するため、1 枚で足りる。
		Microsoft::WRL::ComPtr<ID3D12Resource> rawVisibilityResource_;

		/// [EN] Resource state the raw texture is currently in.
		/// [JP] 生のテクスチャの現在のリソース状態。
		D3D12_RESOURCE_STATES rawVisibilityState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless indices of the raw texture's write and read views.
		/// [JP] 生のテクスチャの書き込み用と読み取り用ビューの bindless インデックス。
		Uint32 rawVisibilityUnorderedAccessViewIndex_ = 0;
		Uint32 rawVisibilityShaderResourceViewIndex_ = 0;

		/// [EN] A second read view of the same raw texture whose component mapping shifts gba into rgb, so reading .rgb through it yields the punctual radiance. The DLSS Ray Reconstruction path points the punctual read at this view; through the plain view, .rgb would mix the directional visibility into the red channel.
		/// [JP] 同じ生のテクスチャに対する 2 つ目の読み取り用ビュー。コンポーネントの対応を gba から rgb へずらしてあり、このビューで .rgb を読むとパンクチュアルの放射輝度になる。DLSS Ray Reconstruction の経路では、パンクチュアルの読み取り先をこのビューにする。通常のビューで .rgb を読むと、ディレクショナルの可視性が赤チャンネルへ混ざる。
		Uint32 rawPunctualShaderResourceViewIndex_ = 0;

		/// [EN] SVGF temporal history of the directional signal (rg: filtered visibility and its variance), two slots per view. This is the feedback tap written by A-Trous step 2 and read by next frame's reprojection, not the final image.
		/// [JP] ディレクショナル信号の SVGF の時間方向の履歴（rg: フィルタ済みの可視性とその分散）。ビューごとに 2 スロット。A-Trous ステップ 2 が書き、次フレームのリプロジェクションが読むフィードバックタップで、最終画ではない。
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalAccumulatedResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES directionalAccumulatedState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 directionalAccumulatedUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 directionalAccumulatedShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] SVGF temporal history of the punctual signal (rgb: filtered radiance, a: its luminance variance), with the same slots and role as the directional history.
		/// [JP] パンクチュアル信号の SVGF の時間方向の履歴（rgb: フィルタ済みの放射輝度、a: その輝度の分散）。スロットと役割はディレクショナルの履歴と同じ。
		Microsoft::WRL::ComPtr<ID3D12Resource> punctualAccumulatedResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES punctualAccumulatedState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 punctualAccumulatedUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 punctualAccumulatedShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] First and second luminance moments of the directional signal (its luminance is the visibility itself). SVGF derives variance from the temporally accumulated moments, so they carry over between frames exactly like the signal.
		/// [JP] ディレクショナル信号の 1 次と 2 次の輝度モーメント（輝度は可視性そのもの）。SVGF は時間方向に蓄積したモーメントから分散を求めるため、信号と同じくフレームをまたいで引き継ぐ。
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalMomentsResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES directionalMomentsState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 directionalMomentsUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 directionalMomentsShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] First and second luminance moments of the punctual radiance.
		/// [JP] パンクチュアルの放射輝度の 1 次と 2 次の輝度モーメント。
		Microsoft::WRL::ComPtr<ID3D12Resource> punctualMomentsResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES punctualMomentsState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 punctualMomentsUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 punctualMomentsShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Per-pixel count of successfully reprojected frames, shared by both signals because the reprojection test is purely geometric. It drives the max(alpha, 1 / length) blend factor and the switch to the spatial variance estimate.
		/// [JP] ピクセルごとのリプロジェクションに成功したフレーム数。リプロジェクションの判定は純粋に幾何的なため、両信号で共有する。max(alpha, 1 / 履歴長) のブレンド係数と、空間的な分散の推定への切り替えを決める。
		Microsoft::WRL::ComPtr<ID3D12Resource> historyLengthResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES historyLengthState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 historyLengthUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 historyLengthShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] This frame's surface packed as (view depth, depth derivative, octahedral normal), shared by both signals. The temporal consistency test compares against the previous frame's copy, which the single-buffered G-Buffer cannot provide. It is 32-bit because SVGF compares depth differences in units of the depth derivative, and FP16 depth is coarser than that derivative at mid distances.
		/// [JP] 今フレームの面を (ビュー深度, 深度の勾配, 八面体の法線) で詰めたもの。両信号で共有する。時間方向の整合性の判定は前フレームのコピーと比べるが、単一バッファの G-Buffer では前フレームを読めない。SVGF は深度差を深度の勾配を単位として比べるが、FP16 の深度は中距離でその勾配より粗くなるため、32 ビットにしている。
		Microsoft::WRL::ComPtr<ID3D12Resource> depthNormalResource_[viewCount_][accumulationSlotCount_];
		D3D12_RESOURCE_STATES depthNormalState_[viewCount_][accumulationSlotCount_] = {};
		Uint32 depthNormalUnorderedAccessViewIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 depthNormalShaderResourceViewIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Fully filtered directional visibility (r) that deferred lighting samples: the output of the last A-Trous iteration, one per view. It never feeds back, which is what lets the history stop at the earlier, sharper feedback tap.
		/// [JP] ディファードライティングがサンプルする、完全にフィルタ済みのディレクショナルの可視性（r）。最後の A-Trous 反復の出力で、ビューごとに 1 枚。フィードバックしないため、履歴をより早くシャープなフィードバックタップで止められる。
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalDenoisedResource_[viewCount_];
		D3D12_RESOURCE_STATES directionalDenoisedState_[viewCount_] = {};
		Uint32 directionalDenoisedUnorderedAccessViewIndex_[viewCount_] = {};
		Uint32 directionalDenoisedShaderResourceViewIndex_[viewCount_] = {};

		/// [EN] Fully filtered punctual radiance (rgb) that deferred lighting samples, one per view.
		/// [JP] ディファードライティングがサンプルする、完全にフィルタ済みのパンクチュアルの放射輝度（rgb）。ビューごとに 1 枚。
		Microsoft::WRL::ComPtr<ID3D12Resource> punctualDenoisedResource_[viewCount_];
		D3D12_RESOURCE_STATES punctualDenoisedState_[viewCount_] = {};
		Uint32 punctualDenoisedUnorderedAccessViewIndex_[viewCount_] = {};
		Uint32 punctualDenoisedShaderResourceViewIndex_[viewCount_] = {};

		/// [EN] A-Trous scratch pair of the directional signal, one pair per view. Always fully overwritten by the pass that writes it.
		/// [JP] ディレクショナル信号の A-Trous 用スクラッチ 2 枚。ビューごとに 1 組。書き込むパスが必ず全画素を上書きする。
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalAtrousScratchResource_[viewCount_][2];
		D3D12_RESOURCE_STATES directionalAtrousScratchState_[viewCount_][2] = {};
		Uint32 directionalAtrousScratchUnorderedAccessViewIndex_[viewCount_][2] = {};
		Uint32 directionalAtrousScratchShaderResourceViewIndex_[viewCount_][2] = {};

		/// [EN] A-Trous scratch pair of the punctual signal, one pair per view.
		/// [JP] パンクチュアル信号の A-Trous 用スクラッチ 2 枚。ビューごとに 1 組。
		Microsoft::WRL::ComPtr<ID3D12Resource> punctualAtrousScratchResource_[viewCount_][2];
		D3D12_RESOURCE_STATES punctualAtrousScratchState_[viewCount_][2] = {};
		Uint32 punctualAtrousScratchUnorderedAccessViewIndex_[viewCount_][2] = {};
		Uint32 punctualAtrousScratchShaderResourceViewIndex_[viewCount_][2] = {};

		/// [EN] Slot holding the previous frame's result, i.e. this frame's history; the other slot is written this frame. It swaps once per frame in Prepare, not in Dispatch, which runs once per view and must see the same assignment each time.
		/// [JP] 前フレームの結果、つまり今フレームの履歴を持つスロット。もう一方に今フレーム書き込む。入れ替えは 1 フレームに 1 回 Prepare で行い、ビューごとに走って毎回同じ割り当てを見る必要がある Dispatch では行わない。
		Uint32 historySlot_ = 0;

		/// [EN] Non-shader-visible heap holding the CPU-side write views that ClearUnorderedAccessViewFloat requires: the raw texture, each view's denoised outputs, and every buffer of the history chain.
		/// [JP] ClearUnorderedAccessViewFloat が要求する CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。生のテクスチャ、ビューごとのデノイズ済み出力、履歴チェーンのすべてのバッファの分。
		DescriptorHeap clearHeap_;

		/// [EN] Indices of the clearable textures' write views inside clearHeap_.
		/// [JP] clearHeap_ 内での、クリアするテクスチャの書き込み用ビューのインデックス。
		Uint32 clearRawIndex_ = 0;
		Uint32 clearDirectionalDenoisedIndex_[viewCount_] = {};
		Uint32 clearPunctualDenoisedIndex_[viewCount_] = {};
		Uint32 clearHistoryLengthIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearDirectionalAccumulatedIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearPunctualAccumulatedIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearDirectionalMomentsIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearPunctualMomentsIndex_[viewCount_][accumulationSlotCount_] = {};
		Uint32 clearDepthNormalIndex_[viewCount_][accumulationSlotCount_] = {};

		/// [EN] Whether the history chain has been zeroed since it was allocated. A new committed resource is not guaranteed to read as zero, and every history buffer feeds back into itself, so an uninitialized texel would persist rather than fade.
		/// [JP] 確保以降に履歴チェーンを 0 で埋めたか。生成直後の committed リソースが 0 で読める保証は無く、履歴のバッファはすべて自分自身へ戻るため、未初期化のテクセルは消えずに残り続ける。
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
		Bool pipelineStateMissingLogged_ = false;
	};
}
