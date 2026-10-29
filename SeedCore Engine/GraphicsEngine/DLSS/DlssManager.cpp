#include <GraphicsEngine/DLSS/DlssManager.h>
#include <FoundationEngine/Log/SlFail.h>
#include <FoundationEngine/File/FileDirectory.h>
#include <FoundationEngine/Log/Warning.h>

#include <External/DLSS/Include/sl.h>
#include <External/DLSS/Include/sl_helpers.h>
#include <External/DLSS/Include/sl_dlss.h>
#include <External/DLSS/Include/sl_deepdvc.h>
#if SC_RELEASE
#include <External/DLSS/Include/sl_security.h>
#endif

namespace SeedCore
{
	/**
	* [EN]
	* State that uses Streamline types, kept here so the header does not
	* need the Streamline headers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Streamline の型を使う状態。ヘッダーが Streamline のヘッダーを必要と
	* しないよう、ここに置く。
	*/
	class DlssManager::Impl final
	{
	public:
		/// [EN] The seven Ray Reconstruction tags, filled again every frame by RayReconstructionTag.
		/// [JP] レイ再構成の7つのタグ。RayReconstructionTag が毎フレーム詰め直す。
		sl::ResourceTag tags_[7]{};

		/// [EN] Token of the current presentation frame, taken by BeginFrame; null until the first frame starts.
		/// [JP] 今の表示フレームのトークン。BeginFrame で取る。最初のフレームが始まるまでは null。
		sl::FrameToken* currentToken_ = nullptr;
	};

	namespace
	{
		/**
		* [EN]
		* Receives Streamline's log messages and sends only errors to the
		* debugger output.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Streamline のログメッセージを受け取り、エラーだけをデバッガの出力へ送る。
		*/
		void ScDlssErrorCallback(sl::LogType type, const Char* message)
		{
			if (type != sl::LogType::eError)
			{
				return;
			}

			/// [EN] The message is prefixed so DLSS errors stand out in the debugger output.
			/// [JP] デバッガの出力で DLSS のエラーと分かるよう、メッセージに接頭辞を付ける。
			Char buffer[512];
			wsprintfA(buffer, "[DLSS Error]: %s\n", message);
			OutputDebugStringA(buffer);
		}

		/**
		* [EN]
		* Copies an engine matrix into Streamline's float4x4 row by row; both
		* store rows in the same order, so no transpose is needed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンジンの行列を Streamline の float4x4 へ行ごとに写す。どちらも
		* 同じ順で行を持つので、転置は要らない。
		*/
		sl::float4x4 ToSlMatrix(const Matrix& m)
		{
			sl::float4x4 result{};
			result.setRow(0, sl::float4(m._11, m._12, m._13, m._14));
			result.setRow(1, sl::float4(m._21, m._22, m._23, m._24));
			result.setRow(2, sl::float4(m._31, m._32, m._33, m._34));
			result.setRow(3, sl::float4(m._41, m._42, m._43, m._44));
			return result;
		}

		/**
		* [EN]
		* Maps the engine's upscale mode to the Streamline DLSS mode;
		* anything unknown falls back to Balanced.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンジンのアップスケールモードを Streamline の DLSS モードへ対応
		* させる。知らない値は Balanced にする。
		*/
		sl::DLSSMode ToStreamlineDlssMode(UpscaleMode mode)
		{
			switch (mode)
			{
			case UpscaleMode::MaxPerformance:
				return sl::DLSSMode::eMaxPerformance;
			case UpscaleMode::Balanced:
				return sl::DLSSMode::eBalanced;
			case UpscaleMode::MaxQuality:
				return sl::DLSSMode::eMaxQuality;
			case UpscaleMode::UltraPerformance:
				return sl::DLSSMode::eUltraPerformance;
			case UpscaleMode::Dlaa:
				return sl::DLSSMode::eDLAA;
			}
			return sl::DLSSMode::eBalanced;
		}
	}

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
	DlssManager::DlssManager() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the manager. Defined here, where Impl is a complete type,
	* so impl_ can delete it. Streamline is shut down by Finalize, not here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マネージャーを破棄する。impl_ が Impl を delete できるよう、Impl が
	* 完全型になるここで定義する。Streamline の終了は Finalize で行い、
	* ここでは行わない。
	*/
	DlssManager::~DlssManager() = default;

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
	Bool DlssManager::Initialize()
	{
		sl::Result sr{ sl::Result::eOk };

		/// [EN] sl.interposer.dll intercepts every D3D12/Vulkan call the engine makes, so verify it is the genuine NVIDIA-signed binary before slInit loads it - a replaced module at this location could hijack the entire graphics pipeline. Skipped in Debug, since development/self-built SL DLLs are unsigned per the Streamline security guide.
		/// [JP] sl.interposer.dllはエンジンが行う全てのD3D12/Vulkan呼び出しを横取りするため、slInitで読み込む前に本物のNVIDIA署名付きバイナリか検証する - ここが差し替えられるとグラフィックスパイプライン全体を乗っ取られかねない。開発版/自前ビルドのSL DLLは署名が無いため(Streamlineセキュリティガイド)、Debugではスキップする。
#if SC_RELEASE
		std::wstring interposerPath = (FileDirectory::ExecutableDirectory() / L"sl.interposer.dll").wstring();
		if (!sl::security::verifyEmbeddedSignature(interposerPath.c_str()))
		{
			SC_LOG_WARNING("sl.interposer.dllの署名検証に失敗しました - 改ざんされた/非公式のモジュールの可能性があります");
			return false;
		}
#endif

		/// [EN] Every feature the engine can use is requested; one that cannot load is reported by slIsFeatureLoaded below rather than failing slInit.
		/// [JP] エンジンが使いうる機能を全て頼む。読み込めない機能は slInit を失敗させず、下の slIsFeatureLoaded で分かる。
		sl::Feature features[] =
		{
			sl::kFeatureDLSS,
			sl::kFeatureReflex,
			sl::kFeatureDeepDVC,
			sl::kFeatureDLSS_G,
			sl::kFeatureDLSS_RR,
		};

		sl::Preferences preference{};

		/// [EN] Streamline's own console window stays closed; its errors reach the debugger output through ScDlssErrorCallback.
		/// [JP] Streamline 自身のコンソールウィンドウは開かない。エラーは ScDlssErrorCallback を通してデバッガの出力へ届く。
		preference.showConsole = false;
		//preference.showConsole = true;

		/// [EN] Debug builds log everything Streamline does; other builds log nothing.
		/// [JP] Debug ビルドでは Streamline の動作を全てログに出し、それ以外では何も出さない。
#if SC_DEBUG
		//preference.logLevel = sl::LogLevel::eDefault;
		preference.logLevel = sl::LogLevel::eVerbose;
#else
		preference.logLevel = sl::LogLevel::eOff;
#endif

		/// [EN] No extra plugin or log paths: plugins are found next to the executable, and logs go only to the callback.
		/// [JP] プラグインやログの追加パスは指定しない。プラグインは実行ファイルの隣から探し、ログはコールバックにだけ送る。
		preference.pathsToPlugins = nullptr;
		preference.numPathsToPlugins = 0;
		preference.pathToLogsAndData = nullptr;
		preference.logMessageCallback = ScDlssErrorCallback;
		preference.featuresToLoad = features;
		preference.numFeaturesToLoad = static_cast<Uint32>(std::size(features));

		/// [EN] The engine identifies itself to NVIDIA as a custom engine with its version and project ID.
		/// [JP] エンジンは、バージョンとプロジェクト ID を持つ独自エンジンとして NVIDIA に名乗る。
		preference.engine = sl::EngineType::eCustom;
		preference.engineVersion = SC_VERTION;
		preference.projectId = SC_ENCRYPTION_KEY_SEED;
		preference.renderAPI = sl::RenderAPI::eD3D12;

		/// [EN] eDisableCLStateTracking: the engine restores command list state itself. eAllowOTA / eLoadDownloadedPlugins: newer DLSS models delivered over the air may be used. eUseFrameBasedResourceTagging: tags are filed under the frame token (slSetTagForFrame). eDisableDebugText: no on-screen Streamline overlay.
		/// [JP] eDisableCLStateTracking: コマンドリストの状態はエンジン自身が戻す。eAllowOTA / eLoadDownloadedPlugins: オンラインで配信される新しい DLSS モデルを使ってよい。eUseFrameBasedResourceTagging: タグはフレームトークンの下にまとめる(slSetTagForFrame)。eDisableDebugText: 画面に Streamline のオーバーレイを出さない。
		preference.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eAllowOTA | sl::PreferenceFlags::eLoadDownloadedPlugins | sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eDisableDebugText;

		sr = slInit(preference);
		SC_SL_CHECK(sr, "Streamlineエンジンの初期化に失敗しました。アプリケーションIDやプラグインパスを確認してください。");

		/// [EN] Which features actually loaded is recorded, so a missing one is skipped later instead of failing at evaluation.
		/// [JP] 実際に読み込めた機能を記録し、読み込めなかったものは実行時に失敗させず後で飛ばせるようにする。
		sr = slIsFeatureLoaded(sl::kFeatureDLSS, loadResult_.isDlssSR_);
		SC_SL_CHECK(sr, "DLSS機能のロード状態を確認できませんでした。");

		sr = slIsFeatureLoaded(sl::kFeatureReflex, loadResult_.isReflex_);
		SC_SL_CHECK(sr, "Reflex機能のロード状態を確認できませんでした。");

		sr = slIsFeatureLoaded(sl::kFeatureDeepDVC, loadResult_.isDeepDVC_);
		SC_SL_CHECK(sr, "Deep DVC機能のロード状態を確認できませんでした。");

		sr = slIsFeatureLoaded(sl::kFeatureDLSS_G, loadResult_.isDlssFG_);
		SC_SL_CHECK(sr, "DLSS FG機能のロード状態を確認できませんでした。");

		sr = slIsFeatureLoaded(sl::kFeatureDLSS_RR, loadResult_.isDlssRR_);
		SC_SL_CHECK(sr, "DLSS RR機能のロード状態を確認できませんでした。");
		return true;
	}

	/**
	* [EN]
	* Unloads the features that hold GPU resources and shuts Streamline down.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GPU リソースを持つ機能をアンロードし、Streamline を終了する。
	*/
	void DlssManager::Finalize()
	{
		sl::Result sr{ sl::Result::eOk };

		/// [EN] sl.deepdvc (Streamline SDK 2.12.0) does not export a freeResources callback, so this call is a no-op until NVIDIA implements it.
		/// [JP] sl.deepdvc(Streamline SDK 2.12.0)はfreeResourcesコールバックをエクスポートしていないため、NVIDIA側が実装するまではこの呼び出しは何もしない。
		//slFreeResources(sl::kFeatureDeepDVC, sl::ViewportHandle(0));
		//slFreeResources(sl::kFeatureDeepDVC, sl::ViewportHandle(1));

		/// [EN] Unloading a feature releases its GPU resources, so DeepDVC and DLSS-G are unloaded before Streamline shuts down; only a loaded feature can be unloaded.
		/// [JP] 機能をアンロードすると GPU リソースが解放されるので、Streamline を終了する前に DeepDVC と DLSS-G をアンロードする。アンロードできるのは読み込まれている機能だけ。
		if (loadResult_.isDeepDVC_)
		{
			sr = slSetFeatureLoaded(sl::kFeatureDeepDVC, false);
			SC_SL_CHECK(sr, "DeepDVC機能のアンロードに失敗しました。");
		}

		if (loadResult_.isDlssFG_)
		{
			sr = slSetFeatureLoaded(sl::kFeatureDLSS_G, false);
			SC_SL_CHECK(sr, "DLSS FG機能のアンロードに失敗しました。");
		}

		sr = slShutdown();
		SC_SL_CHECK(sr, "Streamlineのシャットダウン中にエラーが発生しました。");
	}

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
	Bool DlssManager::Prepare(ID3D12Device* device)
	{
		sl::Result sr{ sl::Result::eOk };

		sr = slSetD3DDevice(device);
		SC_SL_CHECK(sr, "Direct3DデバイスをStreamlineに登録できませんでした。");

		/// [EN] Support is checked against the adapter the device was created on, identified by its LUID.
		/// [JP] 対応しているかは、デバイスを作ったアダプターに対して確かめる。アダプターは LUID で指定する。
		sl::AdapterInfo dlssData{};
		memset(&dlssData, 0, sizeof(sl::AdapterInfo));

		LUID luid = device->GetAdapterLuid();
		dlssData.deviceLUID = reinterpret_cast<Uint8*>(&luid);
		dlssData.deviceLUIDSizeInBytes = sizeof(LUID);

		/// [EN] slIsFeatureSupported answers with an error result when the GPU cannot run the feature, so the result is recorded rather than checked.
		/// [JP] slIsFeatureSupported は GPU がその機能を動かせないときエラーの結果で答えるので、検査せずに結果として記録する。
		supportResult_.isDlssSR_ = slIsFeatureSupported(sl::kFeatureDLSS, dlssData) == sl::Result::eOk;
		supportResult_.isReflex_ = slIsFeatureSupported(sl::kFeatureReflex, dlssData) == sl::Result::eOk;
		supportResult_.isDeepDVC_ = slIsFeatureSupported(sl::kFeatureDeepDVC, dlssData) == sl::Result::eOk;
		supportResult_.isDlssFG_ = slIsFeatureSupported(sl::kFeatureDLSS_G, dlssData) == sl::Result::eOk;
		supportResult_.isDlssRR_ = slIsFeatureSupported(sl::kFeatureDLSS_RR, dlssData) == sl::Result::eOk;
		return true;
	}

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
	void DlssManager::BeginFrame()
	{
		sl::Result sr = slGetNewFrameToken(impl_->currentToken_);
		SC_SL_CHECK(sr, "フレームトークンの取得に失敗しました");
	}

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
	void DlssManager::RayReconstructionTag(DlssBufferTag tags, ID3D12CommandList* cmdList, ID3D12Resource* outPutBuffer, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight)
	{
		/// [EN] Tags are filed under the frame token, so nothing can be tagged before BeginFrame has taken one.
		/// [JP] タグはフレームトークンの下にまとめるので、BeginFrame がトークンを取る前は何もタグ付けできない。
		if (!impl_->currentToken_)
		{
			return;
		}

		sl::Result sr{ sl::Result::eOk };
		sl::ViewportHandle viewport(viewportIndex);

		/// [EN] Each resource carries the state it is in right now, so Streamline can transition it as needed and put it back.
		/// [JP] 各リソースには今の状態を持たせ、Streamline が必要に応じて遷移させて元に戻せるようにする。
		sl::Resource colorResource           = { sl::ResourceType::eTex2d, tags.colorBuffer_,           static_cast<Uint32>(tags.colorBufferState_) };
		sl::Resource depthResource           = { sl::ResourceType::eTex2d, tags.depthBuffer_,           static_cast<Uint32>(tags.depthBufferState_) };
		sl::Resource velocityResource        = { sl::ResourceType::eTex2d, tags.velocityBuffer_,        static_cast<Uint32>(tags.velocityBufferState_) };
		sl::Resource albedoResource          = { sl::ResourceType::eTex2d, tags.albedoBuffer_,          static_cast<Uint32>(tags.albedoBufferState_) };
		sl::Resource normalRoughnessResource = { sl::ResourceType::eTex2d, tags.normalRoughnessBuffer_, static_cast<Uint32>(tags.normalRoughnessBufferState_) };
		sl::Resource specularAlbedoResource  = { sl::ResourceType::eTex2d, tags.specularAlbedoBuffer_,  static_cast<Uint32>(tags.specularAlbedoBufferState_) };
		sl::Resource outputResource          = { sl::ResourceType::eTex2d, outPutBuffer,                static_cast<Uint32>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS) };

		/// [EN] The inputs are read over the render-resolution region.
		/// [JP] 入力は描画解像度の領域から読む。
		sl::Extent inputSize{};
		inputSize.top = 0;
		inputSize.left = 0;
		inputSize.width = static_cast<Uint32>(tags.width_);
		inputSize.height = static_cast<Uint32>(tags.height_);

		/// [EN] The output is written over the output-resolution region.
		/// [JP] 出力は出力解像度の領域へ書く。
		sl::Extent outputSize{};
		outputSize.top = 0;
		outputSize.left = 0;
		outputSize.width = outputWidth;
		outputSize.height = outputHeight;

		/// [EN] DLSS-G reads the same Depth/MotionVectors buffers at Present() time, after slEvaluateFeature has already returned, so while FG is enabled these two need the wider eValidUntilPresent lifecycle instead of eValidUntilEvaluate.
		/// [JP] DLSS-G(FG)はslEvaluateFeatureが戻った後のPresent()時にも同じDepth/MotionVectorsバッファを読むため、FG有効時はこの2つだけeValidUntilEvaluateではなく、より広いeValidUntilPresentが必要。
		sl::ResourceLifecycle depthMotionVectorsLifecycle = frameGenerationEnabled_ ? sl::ResourceLifecycle::eValidUntilPresent : sl::ResourceLifecycle::eValidUntilEvaluate;

		impl_->tags_[0] = { &colorResource,           sl::kBufferTypeScalingInputColor,  sl::ResourceLifecycle::eValidUntilEvaluate, &inputSize };
		impl_->tags_[1] = { &depthResource,           sl::kBufferTypeDepth,              depthMotionVectorsLifecycle,                &inputSize };
		impl_->tags_[2] = { &velocityResource,        sl::kBufferTypeMotionVectors,      depthMotionVectorsLifecycle,                &inputSize };
		impl_->tags_[3] = { &albedoResource,          sl::kBufferTypeAlbedo,             sl::ResourceLifecycle::eValidUntilEvaluate, &inputSize };
		impl_->tags_[4] = { &normalRoughnessResource, sl::kBufferTypeNormalRoughness,    sl::ResourceLifecycle::eValidUntilEvaluate, &inputSize };
		impl_->tags_[5] = { &specularAlbedoResource,  sl::kBufferTypeSpecularAlbedo,     sl::ResourceLifecycle::eValidUntilEvaluate, &inputSize };
		impl_->tags_[6] = { &outputResource,          sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &outputSize };

		/// [EN] The tags point at locals of this function; Streamline copies them before slSetTagForFrame returns.
		/// [JP] タグはこの関数のローカル変数を指している。Streamline は slSetTagForFrame から戻る前にそれを写し取る。
		sr = slSetTagForFrame(*impl_->currentToken_, viewport, impl_->tags_, static_cast<Uint32>(std::size(impl_->tags_)), cmdList);
		SC_SL_CHECK(sr, "フレームへのタグ設定に失敗しました");
	}

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
	void DlssManager::SuperResolutionTag(DlssBufferTag tags, ID3D12CommandList* cmdList, ID3D12Resource* outPutBuffer, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight)
	{
		/// [EN] Tags are filed under the frame token, so nothing can be tagged before BeginFrame has taken one.
		/// [JP] タグはフレームトークンの下にまとめるので、BeginFrame がトークンを取る前は何もタグ付けできない。
		if (!impl_->currentToken_)
		{
			return;
		}

		sl::ViewportHandle viewport(viewportIndex);

		/// [EN] Super Resolution reads only color, depth and motion vectors; the albedo and normal-roughness guides are for Ray Reconstruction.
		/// [JP] Super Resolution が読むのはカラー・深度・モーションベクトルだけ。アルベドや法線ラフネスはレイ再構成のためのもの。
		sl::Resource colorResource    = { sl::ResourceType::eTex2d, tags.colorBuffer_,    static_cast<Uint32>(tags.colorBufferState_) };
		sl::Resource depthResource    = { sl::ResourceType::eTex2d, tags.depthBuffer_,    static_cast<Uint32>(tags.depthBufferState_) };
		sl::Resource velocityResource = { sl::ResourceType::eTex2d, tags.velocityBuffer_, static_cast<Uint32>(tags.velocityBufferState_) };
		sl::Resource outputResource   = { sl::ResourceType::eTex2d, outPutBuffer,         static_cast<Uint32>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS) };

		/// [EN] The inputs are read over the render-resolution region.
		/// [JP] 入力は描画解像度の領域から読む。
		sl::Extent inputSize{};
		inputSize.top = 0;
		inputSize.left = 0;
		inputSize.width = static_cast<Uint32>(tags.width_);
		inputSize.height = static_cast<Uint32>(tags.height_);

		/// [EN] The output is written over the output-resolution region.
		/// [JP] 出力は出力解像度の領域へ書く。
		sl::Extent outputSize{};
		outputSize.top = 0;
		outputSize.left = 0;
		outputSize.width = outputWidth;
		outputSize.height = outputHeight;

		/// [EN] As with Ray Reconstruction, DLSS-G reads depth and motion vectors again at Present, so while FG is enabled these two stay valid until then.
		/// [JP] レイ再構成と同じく、DLSS-G は Present 時に深度とモーションベクトルをもう一度読むので、FG 有効時はこの2つをそれまで有効にしておく。
		sl::ResourceLifecycle depthMotionVectorsLifecycle = frameGenerationEnabled_ ? sl::ResourceLifecycle::eValidUntilPresent : sl::ResourceLifecycle::eValidUntilEvaluate;

		sl::ResourceTag superResolutionTags[4] =
		{
			{ &colorResource,    sl::kBufferTypeScalingInputColor,  sl::ResourceLifecycle::eValidUntilEvaluate, &inputSize },
			{ &depthResource,    sl::kBufferTypeDepth,              depthMotionVectorsLifecycle,                &inputSize },
			{ &velocityResource, sl::kBufferTypeMotionVectors,      depthMotionVectorsLifecycle,                &inputSize },
			{ &outputResource,   sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &outputSize },
		};

		sl::Result sr = slSetTagForFrame(*impl_->currentToken_, viewport, superResolutionTags, static_cast<Uint32>(std::size(superResolutionTags)), cmdList);
		SC_SL_CHECK(sr, "DLSS-SR用リソースタグの設定に失敗しました");
	}

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
	void DlssManager::FrameGenerationTag(DlssBufferTag tags, ID3D12CommandList* cmdList, ID3D12Resource* hudlessResource, ID3D12Resource* uiColorAlphaResource, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight)
	{
		/// [EN] Tags are filed under the frame token, so nothing can be tagged before BeginFrame has taken one.
		/// [JP] タグはフレームトークンの下にまとめるので、BeginFrame がトークンを取る前は何もタグ付けできない。
		if (!impl_->currentToken_)
		{
			return;
		}

		sl::ViewportHandle viewport(viewportIndex);

		/// [EN] Depth and motion vectors are at render resolution; the scene without UI and the UI layer are at output resolution.
		/// [JP] 深度とモーションベクトルは描画解像度、UI を含まないシーンと UI のレイヤーは出力解像度。
		sl::Resource depthResource           = { sl::ResourceType::eTex2d, tags.depthBuffer_,     static_cast<Uint32>(tags.depthBufferState_) };
		sl::Resource velocityResource        = { sl::ResourceType::eTex2d, tags.velocityBuffer_,  static_cast<Uint32>(tags.velocityBufferState_) };
		sl::Resource hudlessResourceTag      = { sl::ResourceType::eTex2d, hudlessResource,       static_cast<Uint32>(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE) };
		sl::Resource uiColorAlphaResourceTag = { sl::ResourceType::eTex2d, uiColorAlphaResource,  static_cast<Uint32>(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE) };

		sl::Extent inputSize{};
		inputSize.top = 0;
		inputSize.left = 0;
		inputSize.width = static_cast<Uint32>(tags.width_);
		inputSize.height = static_cast<Uint32>(tags.height_);

		sl::Extent outputSize{};
		outputSize.top = 0;
		outputSize.left = 0;
		outputSize.width = outputWidth;
		outputSize.height = outputHeight;

		/// [EN] DLSS-G reads all four at Present, after the frame's evaluations have returned, so each must stay valid until then.
		/// [JP] DLSS-G はフレームの実行が戻った後の Present 時に4つ全てを読むので、どれもそれまで有効でなければならない。
		sl::ResourceTag frameGenerationTags[4] =
		{
			{ &depthResource,           sl::kBufferTypeDepth,           sl::ResourceLifecycle::eValidUntilPresent, &inputSize },
			{ &velocityResource,        sl::kBufferTypeMotionVectors,   sl::ResourceLifecycle::eValidUntilPresent, &inputSize },
			{ &hudlessResourceTag,      sl::kBufferTypeHUDLessColor,    sl::ResourceLifecycle::eValidUntilPresent, &outputSize },
			{ &uiColorAlphaResourceTag, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &outputSize },
		};

		sl::Result sr = slSetTagForFrame(*impl_->currentToken_, viewport, frameGenerationTags, static_cast<Uint32>(std::size(frameGenerationTags)), cmdList);
		SC_SL_CHECK(sr, "DLSS-G用リソースタグの設定に失敗しました");
	}

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
	void DlssManager::Constants(const SceneConstantBuffer& scene, Uint32 viewportIndex, Bool reset)
	{
		/// [EN] Constants are filed under the frame token, so nothing can be set before BeginFrame has taken one.
		/// [JP] 定数はフレームトークンの下にまとめるので、BeginFrame がトークンを取る前は何も設定できない。
		if (!impl_->currentToken_)
		{
			return;
		}

		sl::ViewportHandle viewport(viewportIndex);

		/// [EN] The jitter is the shift in the projection's third row. It is in NDC, where the screen spans 2, so half the screen size turns it into pixels; Y is negated because NDC Y points up while pixel Y points down.
		/// [JP] ジッターは射影行列の3行目のずれ。画面全体が幅 2 になる NDC の値なので、画面サイズの半分を掛けてピクセルにする。NDC の Y は上向き・ピクセルの Y は下向きなので Y の符号を反転する。
		Float width = scene.screenSize_.x;
		Float height = scene.screenSize_.y;
		sl::float2 jitterOffset((scene.projection_._31 - scene.nonJitterProjection_._31) * width * 0.5f, -(scene.projection_._32 - scene.nonJitterProjection_._32) * height * 0.5f);

		/// [EN] Clip space is carried to the previous frame's through world space (the current inverse, then the previous view-projection), and back the other way; DLSS uses these to reproject its history.
		/// [JP] クリップ空間は、ワールド空間を経由して(今の逆行列、次に前フレームのビュー射影)前フレームのクリップ空間へ写し、その逆も作る。DLSS はこれで履歴を再投影する。
		Matrix clipToPrevClip = scene.nonJitterViewProjection_.Invert() * scene.previousNonJitterViewProjection_;
		Matrix prevClipToClip = scene.previousNonJitterViewProjection_.Invert() * scene.nonJitterViewProjection_;

		/// [EN] The camera basis in world space is the axes of the inverse view matrix.
		/// [JP] ワールド空間でのカメラの基底は、ビュー逆行列の軸。
		Vector3 cameraRight = scene.inverseView_.Right();
		Vector3 cameraUp = scene.inverseView_.Up();
		Vector3 cameraForward = scene.inverseView_.Forward();

		sl::Constants constants{};

		/// [EN] The projection is given without jitter; the jitter is passed separately in jitterOffset.
		/// [JP] 射影はジッター無しで渡し、ジッターは jitterOffset で別に渡す。
		constants.cameraViewToClip = ToSlMatrix(scene.nonJitterProjection_);
		constants.clipToCameraView = ToSlMatrix(scene.nonJitterProjection_.Invert());

		/// [EN] No lens distortion follows the projection.
		/// [JP] 射影の後にレンズ歪みはかからない。
		constants.clipToLensClip = ToSlMatrix(Matrix::Identity);
		constants.clipToPrevClip = ToSlMatrix(clipToPrevClip);
		constants.prevClipToClip = ToSlMatrix(prevClipToClip);
		constants.jitterOffset = jitterOffset;

		/// [EN] Scale that turns the stored motion vectors into the form Streamline expects; X is negated to match the direction the engine writes them in.
		/// [JP] 保存しているモーションベクトルを Streamline が期待する形へ直す倍率。エンジンが書く向きに合わせて X を反転する。
		constants.mvecScale = sl::float2(-1.0f, 1.0f);
		constants.cameraPinholeOffset = sl::float2(0.0f, 0.0f);
		constants.cameraPos = sl::float3(scene.cameraPosition_.x, scene.cameraPosition_.y, scene.cameraPosition_.z);
		constants.cameraUp = sl::float3(cameraUp.x, cameraUp.y, cameraUp.z);
		constants.cameraRight = sl::float3(cameraRight.x, cameraRight.y, cameraRight.z);
		constants.cameraFwd = sl::float3(cameraForward.x, cameraForward.y, cameraForward.z);
		constants.cameraNear = scene.nearPlane_;
		constants.cameraFar = scene.farPlane_;
		constants.cameraFOV = scene.fieldOfView_;
		constants.cameraAspectRatio = width / height;

		/// [EN] The depth buffer is reversed Z.
		/// [JP] 深度バッファは反転 Z。
		constants.depthInverted = sl::Boolean::eTrue;

		/// [EN] The motion vectors already contain the camera's own motion, not only the objects'.
		/// [JP] モーションベクトルには、物体の動きだけでなくカメラ自身の動きも入っている。
		constants.cameraMotionIncluded = sl::Boolean::eTrue;

		/// [EN] The motion vectors are 2D, in screen space.
		/// [JP] モーションベクトルはスクリーン空間の 2D。
		constants.motionVectors3D = sl::Boolean::eFalse;
		constants.reset = reset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
		constants.orthographicProjection = sl::Boolean::eFalse;

		/// [EN] The motion vectors are at render resolution, one per pixel, and not dilated.
		/// [JP] モーションベクトルは描画解像度で1ピクセルに1つ、膨張させていない。
		constants.motionVectorsDilated = sl::Boolean::eFalse;

		/// [EN] The motion vectors are computed from the jittered positions, so they include the jitter difference between frames.
		/// [JP] モーションベクトルはジッターのかかった位置から求めているので、フレーム間のジッターの差を含む。
		constants.motionVectorsJittered = sl::Boolean::eTrue;

		sl::Result sr = slSetConstants(constants, *impl_->currentToken_, viewport);
		SC_SL_CHECK(sr, "DLSS共通カメラ定数(sl::Constants)の設定に失敗しました");
	}

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
	void DlssManager::ReflexEnable(Bool enable)
	{
		reflexEnabled_ = enable && loadResult_.isReflex_ && supportResult_.isReflex_;
	}

	/**
	* [EN]
	* Returns whether Reflex is actually on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Reflex が実際に有効かを返す。
	*/
	Bool DlssManager::ReflexEnable()const
	{
		return reflexEnabled_;
	}

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
	void DlssManager::DeepDVCEnable(Bool enable)
	{
		deepDVCEnabled_ = enable && loadResult_.isDeepDVC_ && supportResult_.isDeepDVC_;
	}

	/**
	* [EN]
	* Returns whether DeepDVC is actually on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DeepDVC が実際に有効かを返す。
	*/
	Bool DlssManager::DeepDVCEnable()const
	{
		return deepDVCEnabled_;
	}

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
	void DlssManager::FrameGenerationEnable(Bool enable)
	{
		frameGenerationEnabled_ = enable && loadResult_.isDlssFG_ && supportResult_.isDlssFG_;

		/// [EN] On a machine that cannot run DLSS-G, Streamline has nothing to switch, so it is not asked.
		/// [JP] DLSS-G を動かせない環境では Streamline に切り替えるものが無いので、頼まない。
		if (!loadResult_.isDlssFG_ || !supportResult_.isDlssFG_)
		{
			return;
		}

		sl::Result sr = slSetFeatureLoaded(sl::kFeatureDLSS_G, frameGenerationEnabled_);
		SC_SL_CHECK(sr, "DLSS-Gのロード状態切り替えに失敗しました");
	}

	/**
	* [EN]
	* Returns whether Frame Generation is on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレーム生成が有効かを返す。
	*/
	Bool DlssManager::FrameGenerationEnable()const
	{
		return frameGenerationEnabled_;
	}

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
	void DlssManager::FrameGenerationSuppress(Bool suppress)
	{
		frameGenerationSuppressed_ = suppress;
	}

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
	Bool DlssManager::FrameGenerationVSyncSupported()const
	{
		return frameGenerationVSyncSupported_;
	}

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
	void DlssManager::RayReconstructionEnable(Bool enable)
	{
		rayReconstructionEnabled_ = enable && loadResult_.isDlssRR_ && supportResult_.isDlssRR_;
	}

	/**
	* [EN]
	* Returns whether Ray Reconstruction is actually on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイ再構成が実際に有効かを返す。
	*/
	Bool DlssManager::RayReconstructionEnable()const
	{
		return rayReconstructionEnabled_;
	}

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
	void DlssManager::SuperResolutionEnable(Bool enable)
	{
		superResolutionEnabled_ = enable && loadResult_.isDlssSR_ && supportResult_.isDlssSR_;
	}

	/**
	* [EN]
	* Returns whether Super Resolution is actually on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Super Resolution が実際に有効かを返す。
	*/
	Bool DlssManager::SuperResolutionEnable()const
	{
		return superResolutionEnabled_;
	}

	/**
	* [EN]
	* Chooses whether Reflex also boosts GPU clocks on top of lowering latency.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Reflex が遅延の削減に加えて GPU クロックも引き上げるかを選ぶ。
	*/
	void DlssManager::Reflex(Bool useBoost)
	{
		reflexUseBoost_ = useBoost;
	}

	/**
	* [EN]
	* Sets how strongly DeepDVC enhances vibrance, and how much it boosts saturation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DeepDVC が鮮やかさを強める度合いと、彩度を引き上げる量を設定する。
	*/
	void DlssManager::DeepDVC(Float intensity, Float saturationBoost)
	{
		deepDVCIntensity_ = intensity;
		deepDVCSaturationBoost_ = saturationBoost;
	}

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
	void DlssManager::EvaluateReflex()
	{
		/// [EN] The sleep is tied to the frame token, so nothing runs before BeginFrame has taken one; on a machine that cannot run Reflex, there is nothing to set either.
		/// [JP] 待機はフレームトークンに結び付くので、BeginFrame がトークンを取る前は何もしない。Reflex を動かせない環境でも、設定するものが無いので何もしない。
		if (!impl_->currentToken_ || !loadResult_.isReflex_ || !supportResult_.isReflex_)
		{
			return;
		}

		/// [EN] Off, low latency, or low latency with the GPU clock boost.
		/// [JP] 無効・低遅延・GPU クロックを引き上げる低遅延のどれか。
		sl::ReflexOptions reflexOptions{};
		reflexOptions.mode = reflexEnabled_ ? (reflexUseBoost_ ? sl::ReflexMode::eLowLatencyWithBoost : sl::ReflexMode::eLowLatency) : sl::ReflexMode::eOff;

		sl::Result sr = slReflexSetOptions(reflexOptions);
		SC_SL_CHECK(sr, "Reflexオプションの設定に失敗しました");

		/// [EN] Reflex holds the CPU here just long enough that this frame's work reaches the GPU when the GPU is ready for it.
		/// [JP] Reflex はここで CPU を待たせ、このフレームの処理が GPU の手が空いたちょうどその時に届くようにする。
		sr = slReflexSleep(*impl_->currentToken_);
		SC_SL_CHECK(sr, "Reflexのスリープに失敗しました");
	}

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
	void DlssManager::EvaluateDeepDVC(ID3D12CommandList* cmdList, ID3D12Resource* colorBuffer, Uint32 viewportIndex, Uint32 width, Uint32 height)
	{
		if (!impl_->currentToken_ || !deepDVCEnabled_ || !loadResult_.isDeepDVC_)
		{
			return;
		}

		sl::Result sr{ sl::Result::eOk };
		sl::ViewportHandle viewport(viewportIndex);

		/// [EN] DeepDVC reads and writes the same buffer, so it is tagged once, as the output color, in the unordered-access state.
		/// [JP] DeepDVC は同じバッファを読み書きするので、出力カラーとして1回だけ、アンオーダードアクセス状態でタグ付けする。
		sl::Resource colorResource = { sl::ResourceType::eTex2d, colorBuffer, static_cast<Uint32>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS) };

		sl::Extent extent{};
		extent.top = 0;
		extent.left = 0;
		extent.width = width;
		extent.height = height;

		sl::ResourceTag deepDVCTags[1] =
		{
			{ &colorResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &extent },
		};

		sr = slSetTagForFrame(*impl_->currentToken_, viewport, deepDVCTags, static_cast<Uint32>(std::size(deepDVCTags)), cmdList);
		SC_SL_CHECK(sr, "DeepDVC用リソースタグの設定に失敗しました");

		sl::DeepDVCOptions deepDVCOptions{};
		deepDVCOptions.mode = sl::DeepDVCMode::eOn;
		deepDVCOptions.intensity = deepDVCIntensity_;
		deepDVCOptions.saturationBoost = deepDVCSaturationBoost_;

		sr = slDeepDVCSetOptions(viewport, deepDVCOptions);
		SC_SL_CHECK(sr, "DeepDVCオプションの設定に失敗しました");

		/// [EN] The viewport is the only input; the buffer and options were already given under it.
		/// [JP] 入力はビューポートだけ。バッファとオプションは既にそのビューポートの下で渡してある。
		const sl::BaseStructure* inputs[] =
		{
			&viewport,
		};

		sr = slEvaluateFeature(sl::kFeatureDeepDVC, *impl_->currentToken_, inputs, static_cast<Uint32>(std::size(inputs)), cmdList);
		SC_SL_CHECK(sr, "DeepDVCの実行に失敗しました");
	}

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
	void DlssManager::EvaluateFrameGeneration(Uint32 viewportIndex)
	{
		/// [EN] DLSS-G options can be set only on a machine where the feature is both loaded and supported.
		/// [JP] DLSS-G のオプションを設定できるのは、機能が読み込めていて、かつ対応している環境だけ。
		if (!impl_->currentToken_ || !loadResult_.isDlssFG_ || !supportResult_.isDlssFG_)
		{
			return;
		}

		sl::ViewportHandle viewport(viewportIndex);

		/// [EN] While suppressed, generation is off for the frame but the feature stays loaded, so it resumes at once when the suppression is lifted.
		/// [JP] 抑止中はそのフレームの生成を OFF にするが、機能は読み込んだままなので、抑止を解けばすぐに再開する。
		sl::DLSSGOptions options{};
		options.mode = (frameGenerationEnabled_ && !frameGenerationSuppressed_) ? sl::DLSSGMode::eOn : sl::DLSSGMode::eOff;

		sl::Result sr = slDLSSGSetOptions(viewport, options);
		SC_SL_CHECK(sr, "DLSS-Gオプションの設定に失敗しました");

		/// [EN] bIsVsyncSupportAvailable is only meaningful once DLSS-G has actually evaluated at least once, so a failure here (e.g. before that point) is expected and left silent - frameGenerationVSyncSupported_ simply keeps its previous value.
		/// [JP] bIsVsyncSupportAvailableはDLSS-Gが一度でも実行された後でないと意味を持たないため、それ以前の失敗は想定内として無視する - frameGenerationVSyncSupported_は直前の値を保持したままになる。
		sl::DLSSGState state{};
		if (slDLSSGGetState(viewport, state, &options) == sl::Result::eOk)
		{
			frameGenerationVSyncSupported_ = (state.bIsVsyncSupportAvailable == sl::Boolean::eTrue);
		}
	}

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
	void DlssManager::EvaluateRayReconstruction(ID3D12CommandList* cmdList, const SceneConstantBuffer& scene, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight, UpscaleMode mode)
	{
		if (!impl_->currentToken_)
		{
			return;
		}

		sl::Result sr{ sl::Result::eOk };
		sl::ViewportHandle viewport(viewportIndex);

		sl::DLSSDOptions dlssdOptions{};
		dlssdOptions.mode = ToStreamlineDlssMode(mode);
		dlssdOptions.outputWidth = outputWidth;
		dlssdOptions.outputHeight = outputHeight;

		/// [EN] No sharpening, and the color is passed at its own exposure (pre-exposure 1).
		/// [JP] シャープネスはかけず、カラーはそのままの露出で渡す(事前露出 1)。
		dlssdOptions.sharpness = 0.0f;
		dlssdOptions.preExposure = 1.0f;

		/// [EN] The input color is linear HDR, before tone mapping.
		/// [JP] 入力カラーはトーンマッピング前のリニア HDR。
		dlssdOptions.colorBuffersHDR = sl::Boolean::eTrue;
		dlssdOptions.indicatorInvertAxisX = sl::Boolean::eFalse;
		dlssdOptions.indicatorInvertAxisY = sl::Boolean::eFalse;

		/// [EN] Normal and roughness come packed in one buffer.
		/// [JP] 法線とラフネスは1つのバッファにまとめて渡す。
		dlssdOptions.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::ePacked;
		dlssdOptions.alphaUpscalingEnabled = sl::Boolean::eFalse;

		/// [EN] Ray Reconstruction also takes the view transforms, to bring the world-space normals into view space.
		/// [JP] レイ再構成は、ワールド空間の法線をビュー空間へ持ってくるためにビュー変換も受け取る。
		dlssdOptions.worldToCameraView = ToSlMatrix(scene.view_);
		dlssdOptions.cameraViewToWorld = ToSlMatrix(scene.inverseView_);
		sr = slDLSSDSetOptions(viewport, dlssdOptions);
		SC_SL_CHECK(sr, "DLSS-RRオプションの設定に失敗しました");

		/// [EN] The viewport is the only input; the tags, constants and options were already given under it.
		/// [JP] 入力はビューポートだけ。タグ・定数・オプションは既にそのビューポートの下で渡してある。
		const sl::BaseStructure* inputs[] =
		{
			&viewport,
		};

		sr = slEvaluateFeature(sl::kFeatureDLSS_RR, *impl_->currentToken_, inputs, static_cast<Uint32>(std::size(inputs)), cmdList);
		SC_SL_CHECK(sr, "DLSS-RRの実行に失敗しました");
	}

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
	void DlssManager::EvaluateSuperResolution(ID3D12CommandList* cmdList, Uint32 viewportIndex, Uint32 outputWidth, Uint32 outputHeight, UpscaleMode mode)
	{
		if (!impl_->currentToken_)
		{
			return;
		}

		sl::Result sr{ sl::Result::eOk };
		sl::ViewportHandle viewport(viewportIndex);

		sl::DLSSOptions dlssOptions{};
		dlssOptions.mode = ToStreamlineDlssMode(mode);
		dlssOptions.outputWidth = outputWidth;
		dlssOptions.outputHeight = outputHeight;

		/// [EN] No sharpening, and the color is passed at its own exposure (pre-exposure 1).
		/// [JP] シャープネスはかけず、カラーはそのままの露出で渡す(事前露出 1)。
		dlssOptions.sharpness = 0.0f;
		dlssOptions.preExposure = 1.0f;

		/// [EN] The input color is linear HDR, before tone mapping.
		/// [JP] 入力カラーはトーンマッピング前のリニア HDR。
		dlssOptions.colorBuffersHDR = sl::Boolean::eTrue;
		dlssOptions.indicatorInvertAxisX = sl::Boolean::eFalse;
		dlssOptions.indicatorInvertAxisY = sl::Boolean::eFalse;
		sr = slDLSSSetOptions(viewport, dlssOptions);
		SC_SL_CHECK(sr, "DLSSオプションの設定に失敗しました");

		/// [EN] The viewport is the only input; the tags, constants and options were already given under it.
		/// [JP] 入力はビューポートだけ。タグ・定数・オプションは既にそのビューポートの下で渡してある。
		const sl::BaseStructure* inputs[] =
		{
			&viewport,
		};

		sr = slEvaluateFeature(sl::kFeatureDLSS, *impl_->currentToken_, inputs, static_cast<Uint32>(std::size(inputs)), cmdList);
		SC_SL_CHECK(sr, "DLSSの実行に失敗しました");
	}
}
