#pragma once
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Quality/Upscale.h>
#include <GraphicsEngine/System/SceneConstantSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Which Streamline features slInit actually loaded. A feature that
	* failed to load (missing plugin, unsupported driver) stays false and
	* is never evaluated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* slInit が実際に読み込めた Streamline の機能。読み込めなかった機能
	* (プラグインが無い・ドライバが対応していない)は false のままで、
	* 実行されることはない。
	*/
	struct DlssLoadResult
	{
		/// [EN] DLSS Super Resolution (upscaling).
		/// [JP] DLSS Super Resolution(アップスケーリング)。
		Bool isDlssSR_ = false;

		/// [EN] NVIDIA Reflex (render latency reduction).
		/// [JP] NVIDIA Reflex(描画遅延の削減)。
		Bool isReflex_ = false;

		/// [EN] DeepDVC (AI-based vibrance and saturation enhancement).
		/// [JP] DeepDVC(AI による鮮やかさ・彩度の強調)。
		Bool isDeepDVC_ = false;

		/// [EN] DLSS Frame Generation (DLSS-G).
		/// [JP] DLSS フレーム生成(DLSS-G)。
		Bool isDlssFG_ = false;

		/// [EN] DLSS Ray Reconstruction (DLSS-RR / DLSS-D).
		/// [JP] DLSS レイ再構成(DLSS-RR / DLSS-D)。
		Bool isDlssRR_ = false;
	};

	/**
	* [EN]
	* Which Streamline features the GPU the device was created on
	* supports. A feature can load and still be unsupported (a GPU
	* without Tensor cores), so a feature is used only when it is both
	* loaded and supported.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイスを作った GPU が対応している Streamline の機能。読み込めても
	* 対応していない(Tensor コアの無い GPU)ことがあるので、機能を使うのは
	* 読み込めていて、かつ対応しているときだけ。
	*/
	struct DlssSupportResult
	{
		/// [EN] DLSS Super Resolution (upscaling).
		/// [JP] DLSS Super Resolution(アップスケーリング)。
		Bool isDlssSR_ = false;

		/// [EN] NVIDIA Reflex (render latency reduction).
		/// [JP] NVIDIA Reflex(描画遅延の削減)。
		Bool isReflex_ = false;

		/// [EN] DeepDVC (AI-based vibrance and saturation enhancement).
		/// [JP] DeepDVC(AI による鮮やかさ・彩度の強調)。
		Bool isDeepDVC_ = false;

		/// [EN] DLSS Frame Generation (DLSS-G).
		/// [JP] DLSS フレーム生成(DLSS-G)。
		Bool isDlssFG_ = false;

		/// [EN] DLSS Ray Reconstruction (DLSS-RR / DLSS-D).
		/// [JP] DLSS レイ再構成(DLSS-RR / DLSS-D)。
		Bool isDlssRR_ = false;
	};

	/**
	* [EN]
	* The render-resolution buffers handed to Streamline as inputs, each
	* with the resource state it is in at the time it is tagged, plus the
	* size of the region DLSS reads from them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Streamline に入力として渡す、描画解像度のバッファ一式。それぞれに
	* タグ付けする時点でのリソース状態を持たせ、さらに DLSS がそこから
	* 読む領域のサイズを持つ。
	*/
	struct DlssBufferTag
	{
		/// [EN] Lit HDR color before upscaling.
		/// [JP] アップスケール前の、ライティング済み HDR カラー。
		ID3D12Resource* colorBuffer_ = nullptr;

		/// [EN] Scene depth (reversed Z).
		/// [JP] シーンの深度(反転 Z)。
		ID3D12Resource* depthBuffer_ = nullptr;

		/// [EN] Diffuse albedo, read by Ray Reconstruction to separate lighting from surface color.
		/// [JP] 拡散アルベド。レイ再構成がライティングと表面の色を分けるために読む。
		ID3D12Resource* albedoBuffer_ = nullptr;

		/// [EN] Screen-space motion vectors.
		/// [JP] スクリーン空間のモーションベクトル。
		ID3D12Resource* velocityBuffer_ = nullptr;

		/// [EN] Packed normal and roughness, read by Ray Reconstruction.
		/// [JP] 法線とラフネスをまとめたもの。レイ再構成が読む。
		ID3D12Resource* normalRoughnessBuffer_ = nullptr;

		/// [EN] Specular albedo, read by Ray Reconstruction.
		/// [JP] スペキュラーアルベド。レイ再構成が読む。
		ID3D12Resource* specularAlbedoBuffer_ = nullptr;

		/// [EN] Resource state of colorBuffer_ when it is tagged.
		/// [JP] タグ付けする時点での colorBuffer_ のリソース状態。
		D3D12_RESOURCE_STATES colorBufferState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		/// [EN] Resource state of depthBuffer_ when it is tagged.
		/// [JP] タグ付けする時点での depthBuffer_ のリソース状態。
		D3D12_RESOURCE_STATES depthBufferState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		/// [EN] Resource state of albedoBuffer_ when it is tagged.
		/// [JP] タグ付けする時点での albedoBuffer_ のリソース状態。
		D3D12_RESOURCE_STATES albedoBufferState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		/// [EN] Resource state of velocityBuffer_ when it is tagged.
		/// [JP] タグ付けする時点での velocityBuffer_ のリソース状態。
		D3D12_RESOURCE_STATES velocityBufferState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		/// [EN] Resource state of normalRoughnessBuffer_ when it is tagged.
		/// [JP] タグ付けする時点での normalRoughnessBuffer_ のリソース状態。
		D3D12_RESOURCE_STATES normalRoughnessBufferState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		/// [EN] Resource state of specularAlbedoBuffer_ when it is tagged.
		/// [JP] タグ付けする時点での specularAlbedoBuffer_ のリソース状態。
		D3D12_RESOURCE_STATES specularAlbedoBufferState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		/// [EN] Width of the input region in pixels (the render resolution, before upscaling).
		/// [JP] 入力領域の幅(ピクセル)。アップスケール前の描画解像度。
		Float width_ = ScResolution::SC_HD.Width;

		/// [EN] Height of the input region in pixels (the render resolution, before upscaling).
		/// [JP] 入力領域の高さ(ピクセル)。アップスケール前の描画解像度。
		Float height_ = ScResolution::SC_HD.Height;
	};

	/**
	* [EN]
	* Drives NVIDIA Streamline: loads DLSS, Reflex, DeepDVC, Frame
	* Generation and Ray Reconstruction, hands them the frame's buffers
	* and camera constants, and evaluates them. Every per-frame call works
	* under the frame token taken by BeginFrame, and does nothing before
	* one exists.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* NVIDIA Streamline を動かす。DLSS・Reflex・DeepDVC・フレーム生成・
	* レイ再構成を読み込み、フレームのバッファとカメラ定数を渡して実行する。
	* フレームごとの呼び出しはどれも BeginFrame で取ったフレームトークンの
	* 下で動き、トークンが無いうちは何もしない。
	*/
	class SEEDCORE_API DlssManager
	{
	public:
		/**
		* [EN]
		* Creates the manager with its Streamline-side state; Streamline
		* itself is not touched until Initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Streamline 側の状態を持つマネージャーを作る。Streamline 自体には
		* Initialize まで触れない。
		*/
		DlssManager();

		/**
		* [EN]
		* Destroys the manager. Streamline is shut down by Finalize, not here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* マネージャーを破棄する。Streamline の終了は Finalize で行い、ここでは行わない。
		*/
		~DlssManager();

		/**
		* [EN]
		* Starts Streamline before the D3D12 device is created, asking it to
		* load every feature, and records which ones actually loaded. In
		* Release the Streamline interposer DLL is checked for NVIDIA's
		* signature first, and false is returned if it does not pass.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* D3D12 デバイスを作る前に Streamline を起動して全ての機能の読み込みを
		* 頼み、実際に読み込めたものを記録する。Release では先に Streamline の
		* インターポーザー DLL の NVIDIA 署名を確かめ、通らなければ false を返す。
		*/
		Bool Initialize();

		/**
		* [EN]
		* Unloads the features that hold GPU resources and shuts Streamline down.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GPU リソースを持つ機能をアンロードし、Streamline を終了する。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Hands the created D3D12 device to Streamline and records whether
		* each feature is supported on the device's adapter. An unsupported
		* feature is a result, not a failure, so the engine carries on
		* without it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 作成した D3D12 デバイスを Streamline に渡し、各機能がそのデバイスの
		* アダプターで使えるかを記録する。対応していないのは失敗ではなく判定の
		* 結果なので、エンジンはその機能なしで続行する。
		*/
		Bool Prepare(ID3D12Device* device);

		/**
		* [EN]
		* Takes the frame token for the presentation frame that is starting;
		* every tag, constant and evaluation of this frame is filed under it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 始まる表示フレームのフレームトークンを取る。このフレームのタグ・
		* 定数・実行は全てこのトークンの下にまとめられる。
		*/
		void BeginFrame();

		/**
		* [EN]
		* Tags the render-resolution inputs and the output-resolution
		* target that Ray Reconstruction uses for the given viewport.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定したビューポートでレイ再構成が使う、描画解像度の入力と出力解像度の
		* 書き込み先をタグ付けする。
		*/
		void RayReconstructionTag(DlssBufferTag tags, ID3D12CommandList* cmdList, ID3D12Resource* outPutBuffer, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight);

		/**
		* [EN]
		* Tags the render-resolution color, depth and motion vectors and the
		* output-resolution target that Super Resolution uses for the given
		* viewport.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定したビューポートで Super Resolution が使う、描画解像度のカラー・
		* 深度・モーションベクトルと、出力解像度の書き込み先をタグ付けする。
		*/
		void SuperResolutionTag(DlssBufferTag tags, ID3D12CommandList* cmdList, ID3D12Resource* outPutBuffer, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight);

		/**
		* [EN]
		* Tags the buffers Frame Generation reads at Present: depth, motion
		* vectors, the scene without UI, and the UI with its alpha.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレーム生成が Present 時に読むバッファをタグ付けする。深度・
		* モーションベクトル・UI を含まないシーン・アルファ付きの UI。
		*/
		void FrameGenerationTag(DlssBufferTag tags, ID3D12CommandList* cmdList, ID3D12Resource* hudlessResource, ID3D12Resource* uiColorAlphaResource, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight);

		/**
		* [EN]
		* Hands this frame's camera to Streamline: projection without
		* jitter, the jitter offset in pixels, the transforms to and from the
		* previous frame's clip space, and the camera basis. reset tells DLSS
		* to discard its history, as after a camera cut.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このフレームのカメラを Streamline に渡す。ジッター無しの射影・
		* ピクセル単位のジッター量・前フレームのクリップ空間との相互変換・
		* カメラの基底。reset はカメラが切り替わったときのように、DLSS に
		* 履歴を捨てさせる。
		*/
		void Constants(const SceneConstantBuffer& scene, Uint32 viewportIndex, Bool reset);

	public:
		/**
		* [EN]
		* Turns Reflex on or off; takes effect at the next EvaluateReflex. It
		* turns on only when the feature is both loaded and supported.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Reflex を有効・無効にする。次の EvaluateReflex から効く。ON になるのは
		* 機能が読み込めていて、かつ対応しているときだけ。
		*/
		void ReflexEnable(Bool enable);

		/**
		* [EN]
		* Returns whether Reflex is actually on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Reflex が実際に有効かを返す。
		*/
		Bool ReflexEnable()const;

		/**
		* [EN]
		* Turns DeepDVC on or off; while off, EvaluateDeepDVC does nothing. It
		* turns on only when the feature is both loaded and supported.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DeepDVC を有効・無効にする。無効な間、EvaluateDeepDVC は何もしない。
		* ON になるのは機能が読み込めていて、かつ対応しているときだけ。
		*/
		void DeepDVCEnable(Bool enable);

		/**
		* [EN]
		* Returns whether DeepDVC is actually on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DeepDVC が実際に有効かを返す。
		*/
		Bool DeepDVCEnable()const;

		/**
		* [EN]
		* Turns Frame Generation on or off, loading or unloading the DLSS-G
		* feature so that it holds no GPU resources while off. It turns on
		* only when the feature is both loaded and supported.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレーム生成を有効・無効にする。無効な間は GPU リソースを持たない
		* よう、DLSS-G の機能を読み込み・アンロードする。ON になるのは機能が
		* 読み込めていて、かつ対応しているときだけ。
		*/
		void FrameGenerationEnable(Bool enable);

		/**
		* [EN]
		* Returns whether Frame Generation is actually on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレーム生成が実際に有効かを返す。
		*/
		Bool FrameGenerationEnable()const;

		/**
		* [EN]
		* Pauses Frame Generation without unloading it, for while the
		* buffers it reads are being replaced (a render target or swap
		* chain resize). The enabled setting is kept and returns once the
		* suppression is lifted.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレーム生成を、アンロードせずに一時停止する。フレーム生成が読む
		* バッファを置き換えている間(レンダーターゲットやスワップチェーンの
		* リサイズ)に使う。有効の設定はそのまま残り、抑止を解くと元に戻る。
		*/
		void FrameGenerationSuppress(Bool suppress);

		/**
		* [EN]
		* Returns whether Frame Generation can run with VSync on, as last
		* reported by DLSS-G.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレーム生成を VSync 有効のまま動かせるかを、DLSS-G が最後に報告した
		* 値で返す。
		*/
		Bool FrameGenerationVSyncSupported()const;

		/**
		* [EN]
		* Turns Ray Reconstruction on or off. It turns on only when the
		* feature is both loaded and supported, so asking for it on a
		* machine that cannot run it leaves it off.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイ再構成を有効・無効にする。ON になるのは機能が読み込めていて、
		* かつ対応しているときだけなので、動かせない環境で頼んでも OFF のまま。
		*/
		void RayReconstructionEnable(Bool enable);

		/**
		* [EN]
		* Returns whether Ray Reconstruction is actually on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイ再構成が実際に有効かを返す。
		*/
		Bool RayReconstructionEnable()const;

		/**
		* [EN]
		* Turns Super Resolution on or off. It turns on only when the
		* feature is both loaded and supported, so asking for it on a
		* machine that cannot run it leaves it off.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Super Resolution を有効・無効にする。ON になるのは機能が読み込めて
		* いて、かつ対応しているときだけなので、動かせない環境で頼んでも OFF
		* のまま。
		*/
		void SuperResolutionEnable(Bool enable);

		/**
		* [EN]
		* Returns whether Super Resolution is actually on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Super Resolution が実際に有効かを返す。
		*/
		Bool SuperResolutionEnable()const;

	public:
		/**
		* [EN]
		* Chooses whether Reflex also boosts GPU clocks on top of lowering latency.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Reflex が遅延の削減に加えて GPU クロックも引き上げるかを選ぶ。
		*/
		void Reflex(Bool useBoost);

		/**
		* [EN]
		* Sets how strongly DeepDVC enhances vibrance, and how much it boosts saturation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DeepDVC が鮮やかさを強める度合いと、彩度を引き上げる量を設定する。
		*/
		void DeepDVC(Float intensity, Float saturationBoost);

	public:
		/**
		* [EN]
		* Applies the Reflex mode and lets Reflex sleep the CPU at the start
		* of the frame, so the frame does not queue up ahead of the GPU.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Reflex のモードを反映し、フレームの始めに Reflex に CPU を待機させて、
		* フレームが GPU より先に溜まらないようにする。
		*/
		void EvaluateReflex();

		/**
		* [EN]
		* Runs DeepDVC in place on the output-resolution color buffer, when
		* it is on and loaded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DeepDVC が有効で読み込まれているとき、出力解像度のカラーバッファに
		* その場で DeepDVC をかける。
		*/
		void EvaluateDeepDVC(ID3D12CommandList* cmdList, ID3D12Resource* colorBuffer, Uint32 viewportIndex, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Sets the DLSS-G mode for this frame (on only while enabled and not
		* suppressed) and refreshes whether VSync is supported. The frames
		* themselves are generated by Streamline at Present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このフレームの DLSS-G のモードを設定し(有効かつ抑止されていない間
		* だけ ON)、VSync に対応しているかを更新する。フレームそのものは
		* Present 時に Streamline が生成する。
		*/
		void EvaluateFrameGeneration(Uint32 viewportIndex);

		/**
		* [EN]
		* Sets the Ray Reconstruction options and runs it, denoising and
		* upscaling the tagged inputs into the tagged output in one pass.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイ再構成のオプションを設定して実行し、タグ付けした入力のノイズ除去と
		* アップスケールを1回で行って、タグ付けした出力へ書き込む。
		*/
		void EvaluateRayReconstruction(ID3D12CommandList* cmdList, const SceneConstantBuffer& scene, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight, UpscaleMode mode);

		/**
		* [EN]
		* Runs DLSS Super Resolution, upscaling the tagged input color into
		* the tagged output.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DLSS Super Resolution を実行し、タグ付けした入力カラーをタグ付けした
		* 出力へアップスケールする。
		*/
		void EvaluateSuperResolution(ID3D12CommandList* cmdList, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight, UpscaleMode mode);

	private:
		class Impl;

		/// [EN] Which features slInit loaded; a feature that is not loaded is never evaluated.
		/// [JP] slInit が読み込んだ機能。読み込まれていない機能は実行しない。
		DlssLoadResult loadResult_{};

		/// [EN] Which features the device's GPU supports; a feature is turned on only when it is both loaded and supported.
		/// [JP] デバイスの GPU が対応している機能。機能を ON にするのは、読み込めていて、かつ対応しているときだけ。
		DlssSupportResult supportResult_{};

		/// [EN] Whether Reflex is on; false whenever the feature is not loaded or not supported.
		/// [JP] Reflex が有効か。機能が読み込めていない・対応していないときは常に false。
		Bool reflexEnabled_ = false;

		/// [EN] Whether Reflex also boosts GPU clocks.
		/// [JP] Reflex が GPU クロックも引き上げるか。
		Bool reflexUseBoost_ = false;

		/// [EN] Whether DeepDVC is on; false whenever the feature is not loaded or not supported.
		/// [JP] DeepDVC が有効か。機能が読み込めていない・対応していないときは常に false。
		Bool deepDVCEnabled_ = false;

		/// [EN] How strongly DeepDVC enhances vibrance.
		/// [JP] DeepDVC が鮮やかさを強める度合い。
		Float deepDVCIntensity_ = 0.5f;

		/// [EN] How much DeepDVC boosts saturation.
		/// [JP] DeepDVC が彩度を引き上げる量。
		Float deepDVCSaturationBoost_ = 0.25f;

		/// [EN] Whether Frame Generation is on (the DLSS-G feature is loaded); false whenever the feature is not loaded or not supported.
		/// [JP] フレーム生成が有効か(DLSS-G の機能が読み込まれているか)。機能が読み込めていない・対応していないときは常に false。
		Bool frameGenerationEnabled_ = false;

		/// [EN] Whether Frame Generation is paused while its buffers are being replaced.
		/// [JP] バッファの置き換え中で、フレーム生成を一時停止しているか。
		Bool frameGenerationSuppressed_ = false;

		/// [EN] Whether Frame Generation can run with VSync on, as last reported by DLSS-G.
		/// [JP] フレーム生成を VSync 有効のまま動かせるか。DLSS-G が最後に報告した値。
		Bool frameGenerationVSyncSupported_ = false;

		/// [EN] Whether Ray Reconstruction is on; false whenever the feature is not loaded or not supported.
		/// [JP] レイ再構成が有効か。機能が読み込めていない・対応していないときは常に false。
		Bool rayReconstructionEnabled_ = false;

		/// [EN] Whether Super Resolution is on; false whenever the feature is not loaded or not supported.
		/// [JP] Super Resolution が有効か。機能が読み込めていない・対応していないときは常に false。
		Bool superResolutionEnabled_ = false;

		/// [EN] Streamline-typed state, kept out of this header so that users of the class need not include the Streamline headers.
		/// [JP] Streamline の型を使う状態。クラスの利用側が Streamline のヘッダーを include しなくて済むよう、このヘッダーの外に置く。
		ResourcePtr<Impl> impl_;
	};
}
