#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Graphics/GraphicsPresenter.h>
#include <GraphicsEngine/Graphics/GraphicsProcessor.h>
#include <GraphicsEngine/Graphics/GraphicsScene.h>
#include <GraphicsEngine/Graphics/GraphicsUpdater.h>

#include <FoundationEngine/Log/AftermathCrashTracker.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>

#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12DebugLayer.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/Shader/ShaderHotReload.h>

namespace SeedCore
{
	/**
	* [EN]
	* Every member of Graphics, kept out of Graphics.h so the render graph,
	* the scene renderer and every Feature header stay out of the code that
	* includes it. Graphics' own functions read and write these directly.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Graphics の全メンバー。Graphics.h から外しておくことで、レンダーグラフ、
	* SceneRenderer、全 Feature のヘッダーが、Graphics.h をインクルードする
	* コードから見えないようにする。Graphics の関数がこれを直接読み書きする。
	*/
	struct Graphics::Impl
	{
		/// [EN] Owns the D3D12 context, shader cache, shader hot reload, descriptor heaps, pipeline caches and root signature shared by every graphics service.
		/// [JP] 全グラフィックスサービスで共有する、D3D12 コンテキスト、シェーダーキャッシュ、シェーダーのホットリロード、ディスクリプタヒープ、パイプラインのキャッシュ、ルートシグネチャを所有する。
		ResourcePtr<RenderDevice> device_;

		/// [EN] Owns the swap chain and the screen overlays, and presents each frame; built after the device and released before it.
		/// [JP] スワップチェインと画面オーバーレイを持ち、各フレームを表示する。デバイスの後に生成し、前に解放する。
		ResourcePtr<GraphicsPresenter> presenter_;

		/// [EN] Processes asset data on the GPU outside the render graph (BC7 compression of model and material textures).
		/// [JP] レンダーグラフの外で、アセットのデータを GPU で加工する（モデルとマテリアルのテクスチャの BC7 圧縮）。
		GraphicsProcessor processor_;

		/// [EN] GraphicsEngine's loaders and resources; built after the device and released before it, since its texture, model and skymap resources return descriptors to the device's resource heap.
		/// [JP] GraphicsEngine のローダーとリソース。テクスチャ・モデル・スカイマップのリソースがデバイスのリソースヒープへディスクリプタを返すため、デバイスの後に生成し、前に解放する。
		ResourcePtr<GraphicsModule> module_;

		/// [EN] Advances the per-frame state rendering reads (fonts, movies, effects, shapes, animation, materials); built after the asset module it reads from and released before it.
		/// [JP] 描画が読むフレームごとの状態（フォント、ムービー、エフェクト、形、アニメーション、マテリアル）を進める。読み先のアセットモジュールの後に生成し、前に解放する。
		ResourcePtr<GraphicsUpdater> updater_;

		/// [EN] Collects the views during the frame and renders them once through the render graph; built after the asset module it renders with and released before it.
		/// [JP] フレームの間にビューを集め、レンダーグラフで 1 回描画する。描画に使うアセットモジュールの後に生成し、前に解放する。
		ResourcePtr<GraphicsScene> scene_;
	};

	/**
	* [EN]
	* Creates the member storage; nothing touches the GPU until Initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メンバーの入れ物を作る。Initialize まで GPU には触らない。
	*/
	Graphics::Graphics() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the member storage. GPU resources are released by Finalize beforehand.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メンバーの入れ物を破棄する。GPU リソースは、その前に Finalize が解放する。
	*/
	Graphics::~Graphics() = default;

	/**
	* [EN]
	* Initializes graphics services and the resources shared by every view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフィックスサービスと、全ビューで共有するリソースを初期化する。
	*/
	Bool Graphics::Initialize(const Window& window, const SplashSystem& splashSystem, const GameConfig& gameConfig)
	{
		/// [EN] The debug layer and Aftermath only observe devices created after they are enabled, so both come before the device.
		/// [JP] デバッグレイヤーと Aftermath は有効化後に作成されたデバイスしか監視しないため、どちらもデバイスより先に有効化する。
#if SC_DEBUG
		D3D12DebugLayer::Enable();
		AftermathCrashTracker::Enable();
#endif

		/// [EN] The render device creates the D3D12 context, shader cache, descriptor heaps and root signature that every following resource uses.
		/// [JP] 描画デバイスが、以降のすべてのリソースが使う D3D12 コンテキスト、シェーダーキャッシュ、ディスクリプタヒープ、ルートシグネチャを作る。
		impl_->device_ = MakePtr<RenderDevice>();
		if (!impl_->device_->Initialize())
		{
			return false;
		}
		D3D12Context& context = impl_->device_->Context();
		BindlessHeap& resourceHeap = impl_->device_->ResourceHeap();

		/// [EN] Attach crash tracking to the device that was just created.
		/// [JP] 作成したばかりのデバイスにクラッシュ追跡を関連付ける。
#if SC_DEBUG
		AftermathCrashTracker::Create(context.GetDevice());
#endif

		/// [EN] The presenter creates the swap chain and the screen overlays on the finished device.
		/// [JP] 表示器は、生成済みのデバイスにスワップチェインと画面オーバーレイを作る。
		impl_->presenter_ = MakePtr<GraphicsPresenter>(*impl_->device_);
		if (!impl_->presenter_->Initialize(window, splashSystem))
		{
			return false;
		}

		impl_->processor_.Initialize(*impl_->device_);

		/// [EN] The asset module uploads through the context's queues and transfer lanes, allocates descriptors from the device's resource heap, and compresses model textures with the processor.
		/// [JP] アセットモジュールは、Context のキューと転送レーンでアップロードし、デバイスのリソースヒープからディスクリプタを確保し、加工器でモデルのテクスチャを圧縮する。
		impl_->module_ = MakePtr<GraphicsModule>(context, resourceHeap, impl_->processor_);
		impl_->updater_ = MakePtr<GraphicsUpdater>(*impl_->device_, *impl_->module_);

		/// [EN] DLSS was started by EngineCore before the device existed; its features are created against the finished device.
		/// [JP] DLSS はデバイスより前に EngineCore が起動している。その機能は、生成済みのデバイスに対して作成する。
		DlssManager& dlssManager = Gateway::GetDlssManager();
		if (!dlssManager.Prepare(context.GetDevice()))
		{
			SC_LOG_WARNING("DLSSの準備に失敗しました - DLSS無効で続行します");
			return false;
		}

		impl_->scene_ = MakePtr<GraphicsScene>(*impl_->device_, *impl_->module_);
		impl_->scene_->Initialize();

		/// [EN] Reflex and DeepDVC are set once from the game config.
		/// [JP] Reflex と DeepDVC は、ゲームの設定から 1 度だけ設定する。
		dlssManager.ReflexEnable(gameConfig.useReflex_);
		dlssManager.Reflex(false);
		dlssManager.DeepDVCEnable(gameConfig.useDeepDVC_);
		dlssManager.DeepDVC(0.5f, 0.25f);

		/// [EN] The first settings come from the game config; the editor and the runtime then apply theirs every frame.
		/// [JP] 最初の設定はゲームの設定から取る。その後はエディターとランタイムが毎フレーム自分の設定を反映する。
		ScResolution::ResSize outputSize = ToResSize(gameConfig.resolution_);
		GraphicsSettings settings;
		settings.outputWidth_ = static_cast<Uint32>(outputSize.Width);
		settings.outputHeight_ = static_cast<Uint32>(outputSize.Height);
		settings.upscale_ = gameConfig.upscale_;
		settings.verticalSync_ = gameConfig.vsync_;
		settings.frameGeneration_ = gameConfig.useFrameGeneration_;
		Configure(settings);

		return true;
	}

	/**
	* [EN]
	* Releases graphics resources only after queued GPU work is complete.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キューへ投入済みの GPU 処理が完了した後に、グラフィックスリソースを解放する。
	*/
	void Graphics::Finalize()
	{
		/// [EN] No GPU-owned object may be released while command queues can still reference it.
		/// [JP] コマンドキューが参照しうる間は、GPU 所有オブジェクトを解放してはならない。
		Wait();

		/// [EN] Destroy higher-level users before the device services they depend on.
		/// [JP] 依存しているデバイスサービスより先に、上位の利用者を破棄する。
		if (impl_->scene_)
		{
			impl_->scene_->Finalize();
			impl_->scene_.reset();
		}

		/// [EN] EngineCore has already finalized DLSS, which releases its features before the swap chain it presents through.
		/// [JP] DLSS は、表示に使うスワップチェインより先に機能を解放するので、EngineCore がすでに終了させている。
		if (impl_->presenter_)
		{
			impl_->presenter_->Finalize();
			impl_->presenter_.reset();
		}

		/// [EN] The updater reads the asset module, which returns its resources' descriptors to the device's resource heap, so both go before the device, the updater first.
		/// [JP] 更新器はアセットモジュールを読み、アセットモジュールはリソースのディスクリプタをデバイスのリソースヒープへ返すため、どちらもデバイスより先に、更新器から解放する。
		impl_->updater_.reset();
		impl_->module_.reset();

		/// [EN] Shader-owning services release before the shader cache they compiled from.
		/// [JP] シェーダーを持つサービスは、コンパイル元のシェーダーキャッシュより先に解放する。
		impl_->processor_.Finalize();

		/// [EN] Crash tracking stops while the device still exists, then the device itself is destroyed last, after its shader cache and hot reload drop their compiled shaders.
		/// [JP] クラッシュ追跡はデバイスが存在するうちに停止し、シェーダーキャッシュとホットリロードがコンパイル済みのシェーダーを手放した後、最後にデバイス自体を破棄する。
#if SC_DEBUG
		AftermathCrashTracker::Disable();
#endif
		if (impl_->device_)
		{
			impl_->device_->ShaderCache().Clear();
			impl_->device_->ShaderHotReload().Clear();
			impl_->device_.reset();
		}

		/// [EN] With the device gone, the debug layer reports any D3D12 object still alive.
		/// [JP] デバイス破棄後、デバッグレイヤーがまだ生存している D3D12 オブジェクトを報告する。
#if SC_DEBUG
		D3D12DebugLayer::Report();
#endif
	}

	/**
	* [EN]
	* Synchronizes all command queues with the calling thread.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全コマンドキューを呼び出し元スレッドと同期する。
	*/
	void Graphics::Wait()
	{
		/// [EN] Each queue is signaled independently because work can be submitted to any of them.
		/// [JP] 処理は任意のキューへ投入されるため、各キューを個別に Signal / Wait する。
		impl_->device_->Wait();
	}

	/**
	* [EN]
	* Applies the presentation and editor settings; only a change of frame generation recreates anything.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表示とエディターの設定を反映する。何かを作り直すのは、フレーム生成が変わったときだけ。
	*/
	void Graphics::Configure(const GraphicsSettings& settings)
	{
		impl_->scene_->Configure(settings);
		impl_->updater_->Configure(settings);
		impl_->presenter_->Configure(settings);
	}

	/**
	* [EN]
	* Recreates presentation buffers after the output window size changes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 出力ウィンドウサイズの変更後に、表示用バッファを再作成する。
	*/
	void Graphics::ResizeSwapChain(Uint32 width, Uint32 height)
	{
		impl_->presenter_->Resize(width, height);
	}

	/**
	* [EN]
	* Opens a frame, empties the collected views, and makes the current back buffer writable as a render target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームを開始し、集めたビューを空にして、現在のバックバッファをレンダーターゲットとして書き込み可能にする。
	*/
	void Graphics::Begin()
	{
		impl_->scene_->Begin();
		impl_->device_->Context().BeginFrame();
		impl_->presenter_->Begin();
	}

	/**
	* [EN]
	* Renders the collected views, then selects the current back buffer as the output-merger render target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 集めたビューを描画してから、現在のバックバッファを output-merger のレンダーターゲットとして選択する。
	*/
	void Graphics::Bind()
	{
		impl_->scene_->Render(impl_->presenter_->BackBuffer(), impl_->updater_->Celestial());
		impl_->presenter_->Bind();
	}

	/**
	* [EN]
	* Renders the collected views if nothing has yet, then submits and presents the frame after restoring the back buffer for presentation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* まだ描画していなければ集めたビューを描画し、バックバッファを表示用に戻してからフレームを送信・表示する。
	*/
	void Graphics::End()
	{
		impl_->scene_->Render(impl_->presenter_->BackBuffer(), impl_->updater_->Celestial());

		/// [EN] The back buffer leaves the render-target state before the frame is submitted, and is presented after.
		/// [JP] バックバッファはフレームの送信前に render-target 状態を抜け、送信後に表示する。
		impl_->presenter_->End();
		impl_->device_->Context().EndFrame();
		impl_->presenter_->Present();

		/// [EN] Descriptor reuse advances on every heap only after this frame has been submitted to the GPU; the render graph returns descriptors to all four.
		/// [JP] ディスクリプタの再利用は、このフレームを GPU へ送信した後にだけ、全ヒープで進める。レンダーグラフは 4 つのヒープすべてへディスクリプタを返す。
		impl_->device_->ResourceHeap().Retire();
		impl_->device_->SamplerHeap().Retire();
		impl_->device_->RenderTargetHeap().Retire();
		impl_->device_->DepthStencilHeap().Retire();
	}

	/**
	* [EN]
	* Advances the per-frame state shared by every view, with the shape display's camera frustums at the render aspect ratio.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全ビューで共有するフレームごとの状態を進める。形の表示のカメラの視錐台は、描画の縦横比を使う。
	*/
	void Graphics::Update(Float deltaTime, World& world)
	{
		impl_->updater_->Update(deltaTime, world, impl_->scene_->AspectRatio());
	}

	/**
	* [EN]
	* Adds the editor view to this frame's views.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタービューを、このフレームのビューに足す。
	*/
	void Graphics::EditorRender(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode)
	{
		impl_->scene_->AddEditorView(timer, editorCamera, world, viewMode);
	}

	/**
	* [EN]
	* Resolves the game camera and adds the game view to this frame's views, shown on output at outputRect.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームカメラを解決し、ゲームビューをこのフレームのビューに足す。ビューは output の outputRect に出す。
	*/
	void Graphics::GameRender(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect)
	{
		impl_->scene_->AddGameView(timer, cameraSystem, world, output, outputRect);
	}

	/**
	* [EN]
	* Adds the canvas view to this frame's views.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャンバスビューを、このフレームのビューに足す。
	*/
	void Graphics::CanvasRender(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world)
	{
		impl_->scene_->AddCanvasView(timer, canvasCamera, world);
	}

	/**
	* [EN]
	* Draws the current splash phase directly to the presentation back buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在のスプラッシュフェーズを、表示用バックバッファへ直接描画する。
	*/
	void Graphics::DrawSplashScreen(const SplashSystem& splashSystem)
	{
		/// [EN] Splash rendering owns a complete presentation frame and collects no views, so Bind and End render nothing.
		/// [JP] スプラッシュ描画は完全な表示フレームを所有し、ビューを集めないので、Bind と End は何も描画しない。
		Begin();
		Bind();
		impl_->presenter_->DrawSplash(splashSystem);

		/// [EN] End closes and presents the frame, so the caller skips the normal frame entirely.
		/// [JP] End がフレームを閉じて表示するため、呼び出し側は通常フレームを丸ごと省略する。
		End();
	}

	/**
	* [EN]
	* Returns the owned render device.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 所有する描画デバイスを返す。
	*/
	RenderDevice& Graphics::Device()const
	{
		return *impl_->device_;
	}

	/**
	* [EN]
	* Returns GraphicsEngine's asset module: its loaders and resources.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GraphicsEngine のアセットモジュール（ローダーとリソース）を返す。
	*/
	GraphicsModule& Graphics::Module()const
	{
		return *impl_->module_;
	}

	/**
	* [EN]
	* Returns the per-pass GPU timing of the scene renderer's graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SceneRenderer のグラフの、パスごとの GPU 計測を返す。
	*/
	const GpuProfiler& Graphics::Profiler()const
	{
		return impl_->scene_->Profiler();
	}

	/**
	* [EN]
	* Returns the handles of the views' output textures.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各ビューの出力テクスチャのハンドルを返す。
	*/
	const GraphicsDisplay& Graphics::Display()const
	{
		return impl_->scene_->Display();
	}
}
