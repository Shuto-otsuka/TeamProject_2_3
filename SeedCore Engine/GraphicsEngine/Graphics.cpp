#include <GraphicsEngine/Graphics.h>

#include <FoundationEngine/Log/AftermathCrashTracker.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/Time/WorldTimer.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

#include <GraphicsEngine/Avatar/AvatarMesh.h>
#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/PreviewCamera.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12DebugLayer.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/Font/FontResource.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/System/CameraSystem.h>
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Initializes graphics services and the resources shared by every view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフィックスサービスと、全ビューで共有するリソースを初期化する。
	*/
	Bool Graphics::Initialize(HWND hwnd, Uint32 width, Uint32 height, const BootConfig& bootConfig)
	{
		/// [EN] Keep the presentation extent separately from the renderer's scaled internal extent.
		/// [JP] 表示用のサイズは、Renderer の倍率適用後の内部サイズとは別に保持する。
		width_ = width;
		height_ = height;

		/// [EN] DLSS must be initialized before the renderer chooses its feature path.
		/// [JP] Renderer が機能経路を選ぶ前に、DLSS を初期化する必要がある。
		dlssManager_ = MakePtr<DlssManager>();
		if (!dlssManager_->Initialize())
		{
			return false;
		}

		/// [EN] The debug layer and Aftermath only observe devices created after they are enabled, so both come before the context.
		/// [JP] デバッグレイヤーと Aftermath は有効化後に作成されたデバイスしか監視しないため、どちらも Context より先に有効化する。
#ifdef _DEBUG
		D3D12DebugLayer::Enable();
#endif

		AftermathCrashTracker::Enable();

		/// [EN] The context creates the device and queues required by every following GPU resource.
		/// [JP] Context は、以降のすべての GPU リソースに必要なデバイスとキューを作成する。
		context_ = MakePtr<D3D12Context>();
		if (!context_->Initialize())
		{
			return false;
		}

		/// [EN] Attach crash tracking to the device that was just created.
		/// [JP] 作成したばかりのデバイスにクラッシュ追跡を関連付ける。
		AftermathCrashTracker::Create(context_->GetDevice());

		/// [EN] Presentation resources are created after the device and direct queue exist.
		/// [JP] 表示用リソースは、デバイスと direct キューの生成後に作成する。
		swapChain_ = MakePtr<SwapChain>(static_cast<Float>(width), static_cast<Float>(height));
		if (!swapChain_->Create(context_->GetFactory(), context_->GetDevice(), context_->GetDirectQueue()->GetCommandQueue(), hwnd))
		{
			return false;
		}

		/// [EN] Renderer and utility passes share one shader cache and one bindless descriptor space.
		/// [JP] Renderer とユーティリティパスは、一つのシェーダーキャッシュと bindless ディスクリプタ空間を共有する。
		shaderCache_ = MakePtr<ShaderCache>();

		bc7CompressShader_ = MakePtr<BC7CompressShader>();
		bc7CompressShader_->Create(*shaderCache_, context_->GetDevice());

		bindlessHeap_ = MakePtr<BindlessHeap>();
		if (!bindlessHeap_->Create(context_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536))
		{
			return false;
		}

		/// [EN] DLSS features are created against the finished device.
		/// [JP] DLSS の機能は、生成済みのデバイスに対して作成する。
		if (!dlssManager_->Prepare(context_->GetDevice()))
		{
			SC_LOG_WARNING("DLSSの準備に失敗しました - DLSS無効で続行します");
			return false;
		}

		/// [EN] Publish the DLSS manager so engine code outside Graphics can reach it.
		/// [JP] Graphics 外のエンジンコードから参照できるよう、DLSS マネージャーを公開する。
		Gateway::BindDlssManager(dlssManager_.get());

		/// [EN] Renderer is created after all services it receives by reference are ready.
		/// [JP] Renderer は、参照で受け取る全サービスの準備完了後に作成する。
		renderer_ = MakePtr<Renderer>();
		renderer_->Create(context_->GetDevice(), context_->GetDirectQueue()->GetCommandQueue(), static_cast<Uint32>(swapChain_->BufferCount()), bindlessHeap_.get(), *shaderCache_, nativeWidth_, nativeHeight_);

		/// [EN] Each independently rendered view owns separate scene constant-buffer state.
		/// [JP] 独立して描画される各ビューは、個別のシーン定数バッファ状態を所有する。
		editorSceneSystem_ = MakePtr<SceneSystem>(context_->GetDevice(), bindlessHeap_.get());
		gameSceneSystem_ = MakePtr<SceneSystem>(context_->GetDevice(), bindlessHeap_.get());
		canvasSceneSystem_ = MakePtr<SceneSystem>(context_->GetDevice(), bindlessHeap_.get());

		/// [EN] Screen overlays load their pipelines and fixed images up front; the boot screen's images come from bootConfig.
		/// [JP] 画面オーバーレイはパイプラインと固定画像を先に読み込む。ブート画面の画像は bootConfig から与える。
		splashScreen_.Initialize(context_->GetDevice(), context_->GetDirectQueue(), bindlessHeap_.get());

		bootScreen_.Initialize(context_->GetDevice(), context_->GetDirectQueue(), bindlessHeap_.get(), DXGI_FORMAT_R8G8B8A8_UNORM);
		bootScreen_.LoadImages(bootConfig);

		fadeScreen_.Initialize(context_->GetDevice());

		letterScreen_.Initialize(context_->GetDevice(), bindlessHeap_.get());

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
		if (renderer_)
		{
			renderer_.reset();
			renderer_ = nullptr;
		}

		/// [EN] Root signatures are device-bound cached objects and must not outlive device teardown.
		/// [JP] ルートシグネチャはデバイスに紐付くキャッシュオブジェクトのため、デバイス終了処理を越えて生存させない。
		RootSignature::ClearCache();

		if (editorSceneSystem_)
		{
			editorSceneSystem_.reset();
			editorSceneSystem_ = nullptr;
		}

		if (gameSceneSystem_)
		{
			gameSceneSystem_.reset();
			gameSceneSystem_ = nullptr;
		}

		if (canvasSceneSystem_)
		{
			canvasSceneSystem_.reset();
			canvasSceneSystem_ = nullptr;
		}

		/// [EN] DLSS releases its features before the swap chain it presents through.
		/// [JP] DLSS は、表示に使うスワップチェインより先に機能を解放する。
		if (dlssManager_)
		{
			dlssManager_->Finalize();
			dlssManager_.reset();
			dlssManager_ = nullptr;
		}

		if (swapChain_)
		{
			swapChain_.reset();
			swapChain_ = nullptr;
		}

		/// [EN] Screen overlays release their pipeline state before descriptor and device ownership end.
		/// [JP] 画面オーバーレイは、ディスクリプタとデバイスの所有終了前にパイプライン状態を解放する。
		letterScreen_.Finalize();

		splashScreen_.Finalize();

		bootScreen_.Finalize();

		/// [EN] The bindless heap goes after every owner of its descriptor indices has freed them.
		/// [JP] bindless ヒープは、ディスクリプタインデックスを持つ全所有者が解放した後に破棄する。
		if (bindlessHeap_)
		{
			bindlessHeap_.reset();
			bindlessHeap_ = nullptr;
		}

		/// [EN] Shader-owning services release before the shader cache they compiled from.
		/// [JP] シェーダーを持つサービスは、コンパイル元のシェーダーキャッシュより先に解放する。
		if (bc7CompressShader_)
		{
			bc7CompressShader_.reset();
			bc7CompressShader_ = nullptr;
		}

		if (shaderCache_)
		{
			shaderCache_->Clear();
			shaderCache_.reset();
			shaderCache_ = nullptr;
		}

		fadeScreen_.Finalize();

		/// [EN] Crash tracking stops while the device still exists, then the device itself is destroyed last.
		/// [JP] クラッシュ追跡はデバイスが存在するうちに停止し、その後デバイス自体を最後に破棄する。
		AftermathCrashTracker::Disable();

		if (context_)
		{
			context_.reset();
			context_ = nullptr;
		}

		/// [EN] With the device gone, the debug layer reports any D3D12 object still alive.
		/// [JP] デバイス破棄後、デバッグレイヤーがまだ生存している D3D12 オブジェクトを報告する。
#ifdef _DEBUG
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
		/// [EN] Each queue is signaled independently because resource work can be submitted to any of them.
		/// [JP] リソース処理は任意のキューへ投入されるため、各キューを個別に Signal / Wait する。
		context_->GetDirectQueue()->Signal();
		context_->GetDirectQueue()->Wait();

		context_->GetCopyQueue()->Signal();
		context_->GetCopyQueue()->Wait();

		context_->GetComputeQueue()->Signal();
		context_->GetComputeQueue()->Wait();
	}

	/**
	* [EN]
	* Rebuilds internal render targets at the scale required for presentation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表示に必要な倍率で、内部レンダーターゲットを再構築する。
	*/
	void Graphics::ResizeRenderTarget(Uint32 outputWidth, Uint32 outputHeight, UpscaleMode upscaleMode)
	{
		/// [EN] Frame generation is suppressed while its dependent render targets are being replaced.
		/// [JP] フレーム生成が依存するレンダーターゲットを置き換える間は、フレーム生成を抑止する。
		dlssManager_->FrameGenerationSuppress(true);

		Wait();

		/// [EN] Clamp the scaled internal extent to a valid minimum render size.
		/// [JP] 倍率適用後の内部サイズを、有効な最小レンダーサイズへクランプする。
		Float scale = UpscaleRenderScale(upscaleMode);
		nativeWidth_ = Max<Uint32>(64, static_cast<Uint32>(outputWidth * scale + 0.5f));
		nativeHeight_ = Max<Uint32>(64, static_cast<Uint32>(outputHeight * scale + 0.5f));

		/// [EN] Renderer renders at the native extent and upscales to the output extent.
		/// [JP] Renderer はネイティブサイズで描画し、出力サイズへアップスケールする。
		renderer_->Resize(context_->GetDevice(), bindlessHeap_.get(), *shaderCache_, nativeWidth_, nativeHeight_, outputWidth, outputHeight);

		dlssManager_->FrameGenerationSuppress(false);
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
		/// [EN] Presentation buffers are replaced while frame generation is temporarily inactive.
		/// [JP] 表示用バッファは、フレーム生成を一時停止した状態で置き換える。
		dlssManager_->FrameGenerationSuppress(true);

		Wait();

		width_ = width;
		height_ = height;

		swapChain_->Resize(context_->GetDevice(), static_cast<Float>(width_), static_cast<Float>(height_));

		dlssManager_->FrameGenerationSuppress(false);
	}

	/**
	* [EN]
	* Opens a frame and makes the current back buffer writable as a render target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームを開始し、現在のバックバッファをレンダーターゲットとして書き込み可能にする。
	*/
	void Graphics::Begin()
	{
		/// [EN] Shared preparation is valid only for one presentation frame.
		/// [JP] 共有準備処理が有効なのは、表示フレームごとに一度だけである。
		prepared_ = false;

		context_->BeginFrame();

		/// [EN] The Streamline frame token belongs to the presentation frame, not to a view, so it is acquired once here before any view tags DLSS inputs.
		/// [JP] Streamline のフレームトークンはビューではなく表示フレームに属するため、どのビューが DLSS 入力をタグ付けするより前に、ここで一度だけ取得する。
		dlssManager_->BeginFrame();
		dlssManager_->EvaluateReflex();
		dlssManager_->EvaluateFrameGeneration(1);

		/// [EN] Presentation owns the back buffer before this barrier; rendering owns it until End restores present state.
		/// [JP] このバリア前は表示処理がバックバッファを所有し、End で present 状態へ戻すまで描画処理が所有する。
		D3D12CommandList* cmdList = context_->GetDirectList();
		ID3D12Resource* backBuffer = swapChain_->BackBuffer();

		cmdList->Barrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	/**
	* [EN]
	* Selects the current back buffer as the output-merger render target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在のバックバッファを output-merger のレンダーターゲットとして選択する。
	*/
	void Graphics::Bind()
	{
		/// [EN] cmdList records output-merger state, and renderTargetViewHandle identifies this frame's back buffer.
		/// [JP] cmdList は output-merger 状態を記録し、renderTargetViewHandle はこのフレームのバックバッファを識別する。
		D3D12CommandList* cmdList = context_->GetDirectList();
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = swapChain_->Handle();
		cmdList->Get()->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, nullptr);
	}

	/**
	* [EN]
	* Submits the frame after restoring the back buffer for presentation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* バックバッファを表示用に戻してから、フレームを送信する。
	*/
	void Graphics::End()
	{
		D3D12CommandList* cmdList = context_->GetDirectList();
		ID3D12Resource* backBuffer = swapChain_->BackBuffer();

		/// [EN] Present requires the back buffer to leave the render-target state before command submission.
		/// [JP] 表示処理ではコマンド送信前に、バックバッファが render-target 状態を抜けている必要がある。
		cmdList->Barrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		context_->EndFrame();

		/// [EN] Descriptor reuse advances only after this frame has been submitted to the GPU.
		/// [JP] ディスクリプタの再利用は、このフレームが GPU へ送信された後にのみ進める。
		bindlessHeap_->Retire();
	}

	/**
	* [EN]
	* Builds editor camera constants, prepares shared data, and renders the editor view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディターカメラ定数を構築し、共有データを準備してエディタービューを描画する。
	*/
	void Graphics::EditorRender(WorldTimer& timer, const EditorCamera& editorCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world, ViewMode viewMode, std::span<const Entity> selectedEntities)
	{
		/// [EN] This buffer is the complete camera and viewport contract consumed by editor render passes.
		/// [JP] このバッファは、エディターレンダーパスが消費する完全なカメラおよびビューポート契約である。
		SceneConstantBuffer editorSceneConstantBuffer{};

		/// [EN] Copy current and historical transforms so temporal passes can compare adjacent frames.
		/// [JP] テンポラルパスが隣接フレームを比較できるよう、現在および履歴の変換行列をコピーする。
		editorSceneConstantBuffer.view_ = editorCamera.View();
		editorSceneConstantBuffer.inverseView_ = editorCamera.InverseView();
		editorSceneConstantBuffer.projection_ = editorCamera.Projection();
		editorSceneConstantBuffer.inverseProjection_ = editorCamera.InverseProjection();
		editorSceneConstantBuffer.nonJitterProjection_ = editorCamera.NonJitterProjection();
		editorSceneConstantBuffer.currentViewProjection_ = editorCamera.CurrentViewProjection();
		editorSceneConstantBuffer.previousViewProjection_ = editorCamera.PreviousViewProjection();
		editorSceneConstantBuffer.inverseViewProjection_ = editorCamera.InverseViewProjection();
		editorSceneConstantBuffer.nonJitterViewProjection_ = editorCamera.NonJitterViewProjection();
		editorSceneConstantBuffer.previousNonJitterViewProjection_ = editorCamera.PreviousNonJitterViewProjection();
		/// [EN] Supply camera pose, projection range, debug view mode, and frame timing.
		/// [JP] カメラ姿勢、投影範囲、デバッグビュー、フレーム時刻を渡す。
		editorSceneConstantBuffer.cameraPosition_ = Vector4(editorCamera.Eye().x,editorCamera.Eye().y,editorCamera.Eye().z,1.0f);
		editorSceneConstantBuffer.cameraFocus_ = Vector4(editorCamera.Focus().x,editorCamera.Focus().y,editorCamera.Focus().z,1.0f);
		editorSceneConstantBuffer.fieldOfView_ = editorCamera.Fov();
		editorSceneConstantBuffer.nearPlane_ = editorCamera.Near();
		editorSceneConstantBuffer.farPlane_ = editorCamera.Far();
		editorSceneConstantBuffer.viewMode_ = static_cast<Uint>(viewMode);
		editorSceneConstantBuffer.totalTime_ = timer.TotalTime();
		editorSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		editorSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		editorSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		editorSceneConstantBuffer.displaySize_ = renderer_->PostProcessOutputSize();

		/// [EN] Streaming preparation precedes collider gathering and every editor pass.
		/// [JP] ストリーミング準備は、コライダー収集と全エディターパスより先に行う。
		Prepare(timer.DeltaTime(), loaderSystem, resourceCache, world, selectedEntities, editorSceneConstantBuffer);
		renderer_->GatherColliders(world);

		/// [EN] Upload the view-specific constants before the renderer records editor commands.
		/// [JP] Renderer がエディターコマンドを記録する前に、ビュー固有の定数をアップロードする。
		editorSceneSystem_->Upload(editorSceneConstantBuffer);

		/// [EN] Record the editor passes into the editor view's frame buffer.
		/// [JP] エディターパスを、エディタービューのフレームバッファへ記録する。
		renderer_->BeginEditorFrame(context_->GetDirectList());
		renderer_->EditorFlush(context_->GetDirectList(), editorSceneSystem_.get(), viewMode);
		renderer_->EndEditorFrame(context_->GetDirectList(), editorSceneConstantBuffer);
	}

	/**
	* [EN]
	* Updates the game camera and renders the game view with its active-camera state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームカメラを更新し、そのアクティブカメラ状態でゲームビューを描画する。
	*/
	void Graphics::GameRender(GameTimer& timer, CameraSystem& cameraSystem, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world)
	{
		/// [EN] Prepare shared assets using the camera state available at the frame's entry.
		/// [JP] フレーム開始時点で利用可能なカメラ状態を用いて、共有アセットを準備する。
		Prepare(timer.ScaledDeltaTime(), loaderSystem, resourceCache, world, {}, cameraSystem.GetSceneConstantBuffer());

		/// [EN] CameraSystem resolves the active game camera for the current world and render extent.
		/// [JP] CameraSystem は、現在のワールドとレンダーサイズに対するアクティブゲームカメラを解決する。
		cameraSystem.Update(world, timer, static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));

		/// [EN] Copy the resolved constants so Graphics can append the renderer's final display size.
		/// [JP] Graphics が Renderer の最終表示サイズを追加できるよう、解決済み定数をコピーする。
		SceneConstantBuffer gameSceneConstantBuffer = cameraSystem.GetSceneConstantBuffer();
		gameSceneConstantBuffer.displaySize_ = renderer_->PostProcessOutputSize();

		renderer_->BeginGameFrame(context_->GetDirectList());

		/// [EN] Upload only when a game camera exists; the renderer still completes its frame without one.
		/// [JP] ゲームカメラが存在するときだけアップロードする。存在しなくても Renderer はフレームを完了させる。
		Bool hasActiveCamera = cameraSystem.HasActiveCamera();
		if (hasActiveCamera)
		{
			gameSceneSystem_->Upload(gameSceneConstantBuffer);
		}

		renderer_->GameFlush(context_->GetDirectList(), gameSceneSystem_.get(), timer.ScaledDeltaTime(), hasActiveCamera);

		/// [EN] The scene-transition fade is part of the game image, so it is drawn before the game frame closes.
		/// [JP] シーン遷移のフェードはゲーム画像の一部なので、ゲームフレームを閉じる前に描画する。
		fadeScreen_.Draw(context_->GetDirectList()->Get(), Scene::FadeAlpha(), static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));

		renderer_->EndGameFrame(context_->GetDirectList(), gameSceneConstantBuffer);
	}

	/**
	* [EN]
	* Builds canvas camera constants and renders the canvas-specific view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャンバスカメラ定数を構築し、キャンバス専用ビューを描画する。
	*/
	void Graphics::CanvasRender(WorldTimer& timer, const CanvasCamera& canvasCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world)
	{
		/// [EN] Canvas rendering uses a complete independent scene-constant buffer.
		/// [JP] キャンバス描画では、完全に独立したシーン定数バッファを使用する。
		SceneConstantBuffer canvasSceneConstantBuffer{};

		/// [EN] Copy the canvas camera transforms and temporal history into GPU scene constants.
		/// [JP] キャンバスカメラの変換行列とテンポラル履歴を GPU シーン定数へコピーする。
		canvasSceneConstantBuffer.view_ = canvasCamera.View();
		canvasSceneConstantBuffer.inverseView_ = canvasCamera.InverseView();
		canvasSceneConstantBuffer.projection_ = canvasCamera.Projection();
		canvasSceneConstantBuffer.inverseProjection_ = canvasCamera.InverseProjection();
		canvasSceneConstantBuffer.nonJitterProjection_ = canvasCamera.NonJitterProjection();
		canvasSceneConstantBuffer.currentViewProjection_ = canvasCamera.CurrentViewProjection();
		canvasSceneConstantBuffer.previousViewProjection_ = canvasCamera.PreviousViewProjection();
		canvasSceneConstantBuffer.inverseViewProjection_ = canvasCamera.InverseViewProjection();
		canvasSceneConstantBuffer.nonJitterViewProjection_ = canvasCamera.NonJitterViewProjection();
		/// [EN] Supply the canvas camera's pose, projection range, and frame timing.
		/// [JP] キャンバスカメラの姿勢、投影範囲、フレーム時刻を渡す。
		canvasSceneConstantBuffer.cameraPosition_ = Vector4(canvasCamera.Eye().x, canvasCamera.Eye().y, canvasCamera.Eye().z, 1.0f);
		canvasSceneConstantBuffer.cameraFocus_ = Vector4(canvasCamera.Focus().x, canvasCamera.Focus().y, canvasCamera.Focus().z, 1.0f);
		canvasSceneConstantBuffer.fieldOfView_ = canvasCamera.Fov();
		canvasSceneConstantBuffer.nearPlane_ = canvasCamera.Near();
		canvasSceneConstantBuffer.farPlane_ = canvasCamera.Far();
		canvasSceneConstantBuffer.totalTime_ = timer.TotalTime();
		canvasSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		/// [EN] Canvas output is native-sized, so its display size equals its render size.
		/// [JP] キャンバス出力はネイティブサイズのため、表示サイズはレンダーサイズと等しい。
		canvasSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		canvasSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		canvasSceneConstantBuffer.displaySize_ = canvasSceneConstantBuffer.screenSize_;

		/// [EN] Shared preparation runs here only when no earlier view in this frame has done it.
		/// [JP] 共有準備は、このフレームでまだ他のビューが行っていない場合にのみここで実行される。
		Prepare(timer.DeltaTime(), loaderSystem, resourceCache, world, {}, canvasSceneConstantBuffer);

		canvasSceneSystem_->Upload(canvasSceneConstantBuffer);

		/// [EN] Record the canvas passes into the canvas view's frame buffer.
		/// [JP] キャンバスパスを、キャンバスビューのフレームバッファへ記録する。
		renderer_->BeginCanvasFrame(context_->GetDirectList());
		renderer_->CanvasFlush(context_->GetDirectList(), canvasSceneSystem_.get());
		renderer_->EndCanvasFrame(context_->GetDirectList());
	}

	/**
	* [EN]
	* Gathers timeline-preview assets and renders them with a preview camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイムラインプレビュー用アセットを収集し、プレビューカメラで描画する。
	*/
	void Graphics::TimelineRender(WorldTimer& timer, const PreviewCamera& timelineCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix)
	{
		/// [EN] Resolve the mesh pose at the requested timeline time before recording the preview pass.
		/// [JP] プレビューパスを記録する前に、指定タイムライン時刻のメッシュ姿勢を解決する。
		renderer_->GatherTimelinePreview(loaderSystem, resourceCache, meshAssetId, animationAssetId, time, worldMatrix);

		/// [EN] The preview camera supplies a self-contained scene-constant buffer for this offscreen view.
		/// [JP] プレビューカメラは、このオフスクリーンビュー専用の自己完結したシーン定数バッファを渡す。
		SceneConstantBuffer previewSceneConstantBuffer{};

		/// [EN] Temporal preview passes need both current and previous camera transforms for motion-aware effects.
		/// [JP] テンポラルプレビューパスでは、モーション対応エフェクトのために現在と前回のカメラ変換行列の両方が必要になる。
		previewSceneConstantBuffer.view_ = timelineCamera.View();
		previewSceneConstantBuffer.inverseView_ = timelineCamera.InverseView();
		previewSceneConstantBuffer.projection_ = timelineCamera.Projection();
		previewSceneConstantBuffer.inverseProjection_ = timelineCamera.InverseProjection();
		previewSceneConstantBuffer.nonJitterProjection_ = timelineCamera.NonJitterProjection();
		previewSceneConstantBuffer.currentViewProjection_ = timelineCamera.CurrentViewProjection();
		previewSceneConstantBuffer.previousViewProjection_ = timelineCamera.PreviousViewProjection();
		previewSceneConstantBuffer.inverseViewProjection_ = timelineCamera.InverseViewProjection();
		previewSceneConstantBuffer.nonJitterViewProjection_ = timelineCamera.NonJitterViewProjection();
		/// [EN] Pose, projection range, and time describe the camera-dependent inputs for this preview frame.
		/// [JP] 姿勢、投影範囲、時刻は、このプレビューフレームでカメラに依存する入力を表す。
		previewSceneConstantBuffer.cameraPosition_ = Vector4(timelineCamera.Eye().x, timelineCamera.Eye().y, timelineCamera.Eye().z, 1.0f);
		previewSceneConstantBuffer.cameraFocus_ = Vector4(timelineCamera.Focus().x, timelineCamera.Focus().y, timelineCamera.Focus().z, 1.0f);
		previewSceneConstantBuffer.fieldOfView_ = timelineCamera.Fov();
		previewSceneConstantBuffer.nearPlane_ = timelineCamera.Near();
		previewSceneConstantBuffer.farPlane_ = timelineCamera.Far();
		previewSceneConstantBuffer.totalTime_ = timer.TotalTime();
		previewSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		/// [EN] Tool previews render at the internal native extent and expose that extent as their display size.
		/// [JP] ツールプレビューは内部ネイティブサイズで描画し、そのサイズを表示サイズとして公開する。
		previewSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		previewSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		previewSceneConstantBuffer.displaySize_ = previewSceneConstantBuffer.screenSize_;

		/// [EN] Record the timeline pass into its dedicated frame buffer, then publish its display texture.
		/// [JP] タイムラインパスを専用フレームバッファへ記録してから、表示テクスチャを公開する。
		renderer_->BeginTimelineFrame(context_->GetDirectList());
		renderer_->TimelineFlush(context_->GetDirectList(), previewSceneConstantBuffer);
		renderer_->EndTimelineFrame(context_->GetDirectList());
	}

	/**
	* [EN]
	* Gathers model-transform preview assets and renders them with a preview camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデル変形プレビュー用アセットを収集し、プレビューカメラで描画する。
	*/
	void Graphics::ModelTransformRender(WorldTimer& timer, const PreviewCamera& modelTransformCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix)
	{
		/// [EN] Gather the evaluated model pose used by the transform-editing preview.
		/// [JP] 変形編集プレビューで使用する、評価済みモデル姿勢を収集する。
		renderer_->GatherModelTransformPreview(loaderSystem, resourceCache, meshAssetId, animationAssetId, time, worldMatrix);

		/// [EN] Keep this preview's camera state separate from editor and game scene constants.
		/// [JP] このプレビューのカメラ状態は、エディターおよびゲームのシーン定数から分離して保持する。
		SceneConstantBuffer previewSceneConstantBuffer{};

		/// [EN] Preserve transform history so the preview uses the same temporal contract as normal rendering.
		/// [JP] プレビューが通常描画と同じテンポラル契約を使えるよう、変換行列の履歴を保持する。
		previewSceneConstantBuffer.view_ = modelTransformCamera.View();
		previewSceneConstantBuffer.inverseView_ = modelTransformCamera.InverseView();
		previewSceneConstantBuffer.projection_ = modelTransformCamera.Projection();
		previewSceneConstantBuffer.inverseProjection_ = modelTransformCamera.InverseProjection();
		previewSceneConstantBuffer.nonJitterProjection_ = modelTransformCamera.NonJitterProjection();
		previewSceneConstantBuffer.currentViewProjection_ = modelTransformCamera.CurrentViewProjection();
		previewSceneConstantBuffer.previousViewProjection_ = modelTransformCamera.PreviousViewProjection();
		previewSceneConstantBuffer.inverseViewProjection_ = modelTransformCamera.InverseViewProjection();
		previewSceneConstantBuffer.nonJitterViewProjection_ = modelTransformCamera.NonJitterViewProjection();
		/// [EN] Copy the preview camera's pose, clipping range, and timing values.
		/// [JP] プレビューカメラの姿勢、クリッピング範囲、時刻値をコピーする。
		previewSceneConstantBuffer.cameraPosition_ = Vector4(modelTransformCamera.Eye().x, modelTransformCamera.Eye().y, modelTransformCamera.Eye().z, 1.0f);
		previewSceneConstantBuffer.cameraFocus_ = Vector4(modelTransformCamera.Focus().x, modelTransformCamera.Focus().y, modelTransformCamera.Focus().z, 1.0f);
		previewSceneConstantBuffer.fieldOfView_ = modelTransformCamera.Fov();
		previewSceneConstantBuffer.nearPlane_ = modelTransformCamera.Near();
		previewSceneConstantBuffer.farPlane_ = modelTransformCamera.Far();
		previewSceneConstantBuffer.totalTime_ = timer.TotalTime();
		previewSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		/// [EN] The preview's render and display extents match its native offscreen target.
		/// [JP] プレビューのレンダーサイズと表示サイズは、ネイティブなオフスクリーンターゲットと一致する。
		previewSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		previewSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		previewSceneConstantBuffer.displaySize_ = previewSceneConstantBuffer.screenSize_;

		/// [EN] Render the evaluated pose into the model-transform preview target.
		/// [JP] 評価済みの姿勢を、モデル変形プレビューターゲットへ描画する。
		renderer_->BeginModelTransformFrame(context_->GetDirectList());
		renderer_->ModelTransformFlush(context_->GetDirectList(), previewSceneConstantBuffer);
		renderer_->EndModelTransformFrame(context_->GetDirectList());
	}

	/**
	* [EN]
	* Gathers material-preview assets and renders them with a preview camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マテリアルプレビュー用アセットを収集し、プレビューカメラで描画する。
	*/
	void Graphics::MaterialRender(WorldTimer& timer, const PreviewCamera& materialCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 surfaceAssetId, const Matrix& worldMatrix)
	{
		/// [EN] Gather the selected mesh and surface before the material inspection pass.
		/// [JP] マテリアル検査パスの前に、選択中のメッシュとサーフェスを収集する。
		renderer_->GatherMaterialPreview(loaderSystem, resourceCache, meshAssetId, surfaceAssetId, worldMatrix);

		/// [EN] Material inspection has its own camera constants and output dimensions.
		/// [JP] マテリアル検査は、専用のカメラ定数と出力サイズを持つ。
		SceneConstantBuffer previewSceneConstantBuffer{};

		/// [EN] Populate the transform history required by material and post-process preview passes.
		/// [JP] マテリアルおよびポストプロセスのプレビューパスに必要な変換行列履歴を設定する。
		previewSceneConstantBuffer.view_ = materialCamera.View();
		previewSceneConstantBuffer.inverseView_ = materialCamera.InverseView();
		previewSceneConstantBuffer.projection_ = materialCamera.Projection();
		previewSceneConstantBuffer.inverseProjection_ = materialCamera.InverseProjection();
		previewSceneConstantBuffer.nonJitterProjection_ = materialCamera.NonJitterProjection();
		previewSceneConstantBuffer.currentViewProjection_ = materialCamera.CurrentViewProjection();
		previewSceneConstantBuffer.previousViewProjection_ = materialCamera.PreviousViewProjection();
		previewSceneConstantBuffer.inverseViewProjection_ = materialCamera.InverseViewProjection();
		previewSceneConstantBuffer.nonJitterViewProjection_ = materialCamera.NonJitterViewProjection();
		/// [EN] Describe the material-preview camera pose, lens range, and frame time.
		/// [JP] マテリアルプレビューのカメラ姿勢、レンズ範囲、フレーム時刻を記述する。
		previewSceneConstantBuffer.cameraPosition_ = Vector4(materialCamera.Eye().x, materialCamera.Eye().y, materialCamera.Eye().z, 1.0f);
		previewSceneConstantBuffer.cameraFocus_ = Vector4(materialCamera.Focus().x, materialCamera.Focus().y, materialCamera.Focus().z, 1.0f);
		previewSceneConstantBuffer.fieldOfView_ = materialCamera.Fov();
		previewSceneConstantBuffer.nearPlane_ = materialCamera.Near();
		previewSceneConstantBuffer.farPlane_ = materialCamera.Far();
		previewSceneConstantBuffer.totalTime_ = timer.TotalTime();
		previewSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		/// [EN] Material inspection renders and displays at the preview target's native size.
		/// [JP] マテリアル検査は、プレビューターゲットのネイティブサイズで描画・表示する。
		previewSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		previewSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		previewSceneConstantBuffer.displaySize_ = previewSceneConstantBuffer.screenSize_;

		/// [EN] Render the material preview into its independent display texture.
		/// [JP] マテリアルプレビューを、独立した表示テクスチャへ描画する。
		renderer_->BeginMaterialFrame(context_->GetDirectList());
		renderer_->MaterialFlush(context_->GetDirectList(), previewSceneConstantBuffer);
		renderer_->EndMaterialFrame(context_->GetDirectList());
	}

	/**
	* [EN]
	* Gathers skeleton-controller preview assets and renders the selected-node view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スケルトンコントローラープレビュー用アセットを収集し、選択ノードのビューを描画する。
	*/
	void Graphics::SkeletonControllerRender(WorldTimer& timer, const PreviewCamera& skeletonControllerCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix, Int selectedNodeIndex)
	{
		/// [EN] Gather the evaluated skeleton and retain the selected node for controller visualization.
		/// [JP] 評価済みスケルトンを収集し、コントローラー可視化用に選択ノードを保持する。
		renderer_->GatherSkeletonControllerPreview(loaderSystem, resourceCache, meshAssetId, animationAssetId, time, worldMatrix, selectedNodeIndex);

		/// [EN] Skeleton editing uses a dedicated preview camera and scene constants.
		/// [JP] スケルトン編集では、専用のプレビューカメラとシーン定数を使用する。
		SceneConstantBuffer previewSceneConstantBuffer{};

		/// [EN] Preserve current and previous transforms so controller visualization remains compatible with temporal passes.
		/// [JP] コントローラー可視化がテンポラルパスと互換性を保てるよう、現在と前回の変換行列を保持する。
		previewSceneConstantBuffer.view_ = skeletonControllerCamera.View();
		previewSceneConstantBuffer.inverseView_ = skeletonControllerCamera.InverseView();
		previewSceneConstantBuffer.projection_ = skeletonControllerCamera.Projection();
		previewSceneConstantBuffer.inverseProjection_ = skeletonControllerCamera.InverseProjection();
		previewSceneConstantBuffer.nonJitterProjection_ = skeletonControllerCamera.NonJitterProjection();
		previewSceneConstantBuffer.currentViewProjection_ = skeletonControllerCamera.CurrentViewProjection();
		previewSceneConstantBuffer.previousViewProjection_ = skeletonControllerCamera.PreviousViewProjection();
		previewSceneConstantBuffer.inverseViewProjection_ = skeletonControllerCamera.InverseViewProjection();
		previewSceneConstantBuffer.nonJitterViewProjection_ = skeletonControllerCamera.NonJitterViewProjection();
		/// [EN] Copy the controller camera pose, projection range, and timing information.
		/// [JP] コントローラーカメラの姿勢、投影範囲、時刻情報をコピーする。
		previewSceneConstantBuffer.cameraPosition_ = Vector4(skeletonControllerCamera.Eye().x, skeletonControllerCamera.Eye().y, skeletonControllerCamera.Eye().z, 1.0f);
		previewSceneConstantBuffer.cameraFocus_ = Vector4(skeletonControllerCamera.Focus().x, skeletonControllerCamera.Focus().y, skeletonControllerCamera.Focus().z, 1.0f);
		previewSceneConstantBuffer.fieldOfView_ = skeletonControllerCamera.Fov();
		previewSceneConstantBuffer.nearPlane_ = skeletonControllerCamera.Near();
		previewSceneConstantBuffer.farPlane_ = skeletonControllerCamera.Far();
		previewSceneConstantBuffer.totalTime_ = timer.TotalTime();
		previewSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		/// [EN] The skeleton-controller preview uses its native render target as the displayed image.
		/// [JP] スケルトンコントローラープレビューは、ネイティブなレンダーターゲットをそのまま表示画像として使用する。
		previewSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		previewSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		previewSceneConstantBuffer.displaySize_ = previewSceneConstantBuffer.screenSize_;

		/// [EN] Render the controller overlay and model into the skeleton preview target.
		/// [JP] コントローラーオーバーレイとモデルを、スケルトンプレビューターゲットへ描画する。
		renderer_->BeginSkeletonControllerFrame(context_->GetDirectList());
		renderer_->SkeletonControllerFlush(context_->GetDirectList(), previewSceneConstantBuffer);
		renderer_->EndSkeletonControllerFrame(context_->GetDirectList());
	}

	/**
	* [EN]
	* Gathers avatar preview geometry and renders it with a preview camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アバタープレビューのジオメトリを収集し、プレビューカメラで描画する。
	*/
	void Graphics::AvatarRender(WorldTimer& timer, const PreviewCamera& avatarCamera, const AvatarMesh& mesh, Uint32 boneCount, const Matrix& worldMatrix, std::span<const Uint32> regionTextureIndices)
	{
		/// [EN] Upload avatar skinning and region-texture selections before drawing the avatar preview.
		/// [JP] アバタープレビューを描画する前に、スキニング情報とリージョンテクスチャ選択をアップロードする。
		renderer_->GatherAvatarPreview(mesh, boneCount, worldMatrix, regionTextureIndices);

		/// [EN] Avatar rendering receives an isolated camera buffer, just like the other tool previews.
		/// [JP] アバター描画は、ほかのツールプレビューと同様に独立したカメラバッファを受け取る。
		SceneConstantBuffer previewSceneConstantBuffer{};

		/// [EN] Store transform history to support the same motion-sensitive rendering features as other views.
		/// [JP] 他ビューと同じモーション依存の描画機能をサポートするため、変換行列履歴を格納する。
		previewSceneConstantBuffer.view_ = avatarCamera.View();
		previewSceneConstantBuffer.inverseView_ = avatarCamera.InverseView();
		previewSceneConstantBuffer.projection_ = avatarCamera.Projection();
		previewSceneConstantBuffer.inverseProjection_ = avatarCamera.InverseProjection();
		previewSceneConstantBuffer.nonJitterProjection_ = avatarCamera.NonJitterProjection();
		previewSceneConstantBuffer.currentViewProjection_ = avatarCamera.CurrentViewProjection();
		previewSceneConstantBuffer.previousViewProjection_ = avatarCamera.PreviousViewProjection();
		previewSceneConstantBuffer.inverseViewProjection_ = avatarCamera.InverseViewProjection();
		previewSceneConstantBuffer.nonJitterViewProjection_ = avatarCamera.NonJitterViewProjection();
		/// [EN] Pass the avatar camera's pose, clipping range, and frame timing to the GPU.
		/// [JP] アバターカメラの姿勢、クリッピング範囲、フレーム時刻を GPU へ渡す。
		previewSceneConstantBuffer.cameraPosition_ = Vector4(avatarCamera.Eye().x, avatarCamera.Eye().y, avatarCamera.Eye().z, 1.0f);
		previewSceneConstantBuffer.cameraFocus_ = Vector4(avatarCamera.Focus().x, avatarCamera.Focus().y, avatarCamera.Focus().z, 1.0f);
		previewSceneConstantBuffer.fieldOfView_ = avatarCamera.Fov();
		previewSceneConstantBuffer.nearPlane_ = avatarCamera.Near();
		previewSceneConstantBuffer.farPlane_ = avatarCamera.Far();
		previewSceneConstantBuffer.totalTime_ = timer.TotalTime();
		previewSceneConstantBuffer.deltaTime_ = timer.DeltaTime();
		/// [EN] Avatar preview rendering and display use the same native offscreen extent.
		/// [JP] アバタープレビューの描画と表示は、同じネイティブなオフスクリーンサイズを使用する。
		previewSceneConstantBuffer.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		previewSceneConstantBuffer.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		previewSceneConstantBuffer.displaySize_ = previewSceneConstantBuffer.screenSize_;

		/// [EN] Render the skinned avatar into the avatar preview display texture.
		/// [JP] スキニング済みアバターを、アバタープレビュー表示テクスチャへ描画する。
		renderer_->BeginAvatarFrame(context_->GetDirectList());
		renderer_->AvatarFlush(context_->GetDirectList(), previewSceneConstantBuffer);
		renderer_->EndAvatarFrame(context_->GetDirectList());
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
		/// [EN] Splash rendering owns a complete presentation frame because it bypasses normal view composition.
		/// [JP] スプラッシュ描画は通常のビュー合成を経由しないため、完全な表示フレームを所有する。
		Begin();
		Bind();

		/// [EN] These native handles are shared by the boot and logo draw paths.
		/// [JP] これらのネイティブハンドルは、ブート画面とロゴ描画経路で共有する。
		ID3D12GraphicsCommandList6* cmdList = context_->GetDirectList()->Get();
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = swapChain_->Handle();

		/// [EN] Boot uses configurable progress visuals; earlier phases use fixed splash imagery.
		/// [JP] ブートでは設定可能な進行表示を使い、それ以前のフェーズでは固定スプラッシュ画像を使う。
		if (splashSystem.Phase() == SplashPhase::Boot)
		{
			bootScreen_.Draw(cmdList, renderTargetViewHandle, splashSystem.Config(), static_cast<Float>(width_), static_cast<Float>(height_), splashSystem.Progress(), splashSystem.Time(), 1.0f);
		}
		else
		{
			splashScreen_.Draw(cmdList, renderTargetViewHandle, static_cast<Float>(width_), static_cast<Float>(height_), splashSystem.Phase(), splashSystem.Alpha());
		}

		/// [EN] Close and present the frame here, so the caller skips the normal frame entirely.
		/// [JP] ここでフレームを閉じて表示するため、呼び出し側は通常フレームを丸ごと省略する。
		End();
		swapChain_->Present(context_->GetDevice());
	}

	/**
	* [EN]
	* Composites the letterbox overlay onto the currently rendered game image.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在描画済みのゲーム画像へ、レターボックスオーバーレイを合成する。
	*/
	void Graphics::DrawLetterScreen()
	{
		/// [EN] The game image is fitted into the swap chain extent, keeping its aspect ratio.
		/// [JP] ゲーム画像を、アスペクト比を保ったままスワップチェインのサイズへ収める。
		letterScreen_.Draw(context_->GetDirectList()->Get(), renderer_->GameDisplayResource(), swapChain_->Handle(), static_cast<Float>(width_), static_cast<Float>(height_));
	}

	/**
	* [EN]
	* Forwards the complete raytracing configuration to Renderer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 完全なレイトレーシング設定を Renderer へ転送する。
	*/
	void Graphics::Raytracing(const RaytracingContext& settings)
	{
		renderer_->Raytracing(settings);
	}

	/**
	* [EN]
	* Selects the renderer upscaling implementation for subsequent views.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 後続ビューに使用する Renderer のアップスケール実装を選択する。
	*/
	void Graphics::Upscale(Bool dlssRayReconstructionEnabled, UpscaleMode upscaleMode)
	{
		renderer_->Upscale(dlssRayReconstructionEnabled, upscaleMode);
	}

	/**
	* [EN]
	* Updates Reflex enablement and boost policy through the DLSS manager.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DLSS マネージャーを通じて Reflex の有効状態とブースト方針を更新する。
	*/
	void Graphics::Reflex(Bool enable, Bool useBoost)
	{
		dlssManager_->ReflexEnable(enable);
		dlssManager_->Reflex(useBoost);
	}

	/**
	* [EN]
	* Updates DeepDVC enablement and image-enhancement tuning values.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DeepDVC の有効状態と、画像強調用の調整値を更新する。
	*/
	void Graphics::DeepDVC(Bool enable, Float intensity, Float saturationBoost)
	{
		dlssManager_->DeepDVCEnable(enable);
		dlssManager_->DeepDVC(intensity, saturationBoost);
	}

	/**
	* [EN]
	* Changes frame generation only after GPU work completes and presentation resources are rebuilt.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GPU 処理の完了と表示用リソースの再構築後にのみ、フレーム生成を変更する。
	*/
	void Graphics::FrameGeneration(Bool enable)
	{
		/// [EN] Avoid rebuilding the swap chain when the requested state is already active.
		/// [JP] 要求状態が既に有効なら、スワップチェインを再構築しない。
		if (dlssManager_->FrameGenerationEnable() == enable)
		{
			return;
		}

		/// [EN] Preserve the native window while the swap chain is destroyed and recreated.
		/// [JP] スワップチェインを破棄・再作成する間も、ネイティブウィンドウを保持する。
		HWND hwnd = swapChain_->GetHwnd();

		Wait();

		/// [EN] Frame generation hooks the swap chain at creation, so the state is switched while no swap chain exists.
		/// [JP] フレーム生成はスワップチェイン作成時にフックするため、スワップチェインが存在しない間に状態を切り替える。
		swapChain_->Destroy();

		dlssManager_->FrameGenerationEnable(enable);

		swapChain_->Create(context_->GetFactory(), context_->GetDevice(), context_->GetDirectQueue()->GetCommandQueue(), hwnd);
	}

	/**
	* [EN]
	* Stores the presentation synchronization preference in SwapChain.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表示同期の設定を SwapChain へ保存する。
	*/
	void Graphics::VerticalSync(Bool vsync)
	{
		swapChain_->VerticalSync(vsync);
	}

	/**
	* [EN]
	* Reads the presentation synchronization preference from SwapChain.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表示同期の設定を SwapChain から取得する。
	*/
	Bool Graphics::VerticalSync()const
	{
		return swapChain_->VerticalSync();
	}

	/**
	* [EN]
	* Returns the owned D3D12 context.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 所有する D3D12 コンテキストを返す。
	*/
	D3D12Context& Graphics::GetContext()const
	{
		return *context_;
	}

	/**
	* [EN]
	* Returns the owned swap chain.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 所有するスワップチェインを返す。
	*/
	SwapChain& Graphics::GetSwapChain()const
	{
		return *swapChain_;
	}

	/**
	* [EN]
	* Returns the bindless descriptor heap shared by graphics services.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフィックスサービスで共有する bindless ディスクリプタヒープを返す。
	*/
	BindlessHeap& Graphics::GetBindlessHeap()const
	{
		return *bindlessHeap_;
	}

	/**
	* [EN]
	* Returns the GPU BC7 compression service.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GPU BC7 圧縮サービスを返す。
	*/
	BC7CompressShader& Graphics::GetBC7CompressShader()const
	{
		return *bc7CompressShader_;
	}

	/**
	* [EN]
	* Returns renderer-owned GPU profiling data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Renderer が所有する GPU プロファイリングデータを返す。
	*/
	const GpuProfiler& Graphics::GetGpuProfiler()const
	{
		return renderer_->GetGpuProfiler();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the editor view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタービュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::EditorDisplayGPUHandle()const
	{
		return renderer_->EditorDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the game view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::GameDisplayGPUHandle()const
	{
		return renderer_->GameDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the canvas view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャンバスビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::CanvasDisplayGPUHandle()const
	{
		return renderer_->CanvasDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the timeline preview.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイムラインプレビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::TimelineDisplayGPUHandle()const
	{
		return renderer_->TimelineDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the model-transform preview.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデル変形プレビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::ModelTransformDisplayGPUHandle()const
	{
		return renderer_->ModelTransformDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the material preview.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マテリアルプレビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::MaterialDisplayGPUHandle()const
	{
		return renderer_->MaterialDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the skeleton-controller preview.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スケルトンコントローラープレビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::SkeletonControllerDisplayGPUHandle()const
	{
		return renderer_->SkeletonControllerDisplayGPUHandle();
	}

	/**
	* [EN]
	* Returns the shader-visible display handle for the avatar preview.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アバタープレビュー用のシェーダー可視表示ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::AvatarDisplayGPUHandle()const
	{
		return renderer_->AvatarDisplayGPUHandle();
	}

	/**
	* [EN]
	* Updates shared streamed resources and prepares Renderer once for the current frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有ストリーミングリソースを更新し、現在のフレームに対する Renderer 準備を一度だけ行う。
	*/
	void Graphics::Prepare(Float deltaTime, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world, std::span<const Entity> selectedEntities, const SceneConstantBuffer& streamingScene)
	{
		/// [EN] Editor, game, and canvas rendering can share one frame; repeated preparation would update streamed resources twice.
		/// [JP] エディター、ゲーム、キャンバス描画は一つのフレームを共有できるため、準備を繰り返すとストリーミングリソースを二重に更新してしまう。
		if (prepared_)
		{
			return;
		}
		prepared_ = true;

		/// [EN] Resource updates precede Renderer::PrepareFrame so every pass observes the same current asset state.
		/// [JP] 全パスが同じ最新アセット状態を参照できるよう、リソース更新を Renderer::PrepareFrame より先に行う。
		resourceCache.GetResource<FontResource>(AssetType::Font)->Update(loaderSystem, context_->GetDevice(), context_->GetDirectQueue()->GetCommandQueue(), bindlessHeap_.get());

		/// [EN] Movies advance before their GPU texture is consumed by renderer passes.
		/// [JP] ムービーは GPU テクスチャが Renderer パスで消費される前に進める。
		movieSystem_.Update(loaderSystem, world, resourceCache);
		resourceCache.GetResource<MovieResource>(AssetType::Movie)->Update(loaderSystem, context_->GetDevice(), context_->GetDirectList()->Get(), bindlessHeap_.get());

		/// [EN] Renderer receives the selected view's scene constants and entity selection after shared assets are current.
		/// [JP] 共有アセットが最新化された後、Renderer は選択されたビューのシーン定数とエンティティ選択を受け取る。
		renderer_->PrepareFrame(context_->GetDirectList(), loaderSystem, resourceCache, world, streamingScene, deltaTime, selectedEntities);
	}
}
