#include <GraphicsEngine/Graphics/GraphicsScene.h>
#include <GraphicsEngine/Graphics/Graphics.h>

#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/System/CameraSystem.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderGraphMemory.h>
#include <GraphicsEngine/RenderGraph/SceneRenderer.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>

#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/Time/WorldTimer.h>

namespace SeedCore
{
	/**
	* [EN]
	* Keeps the device and the asset module. Nothing touches the GPU until Initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイスとアセットモジュールを受け取る。Initialize まで GPU には触らない。
	*/
	GraphicsScene::GraphicsScene(RenderDevice& device, GraphicsModule& module) :device_(device), module_(module)
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the scene. The scene renderer is released by Finalize beforehand.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンを破棄する。SceneRenderer は、その前に Finalize が解放する。
	*/
	GraphicsScene::~GraphicsScene() = default;

	/**
	* [EN]
	* Creates the scene renderer on the device, and takes the screen's render target view slot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイスに SceneRenderer を作り、画面の RTV の枠を取る。
	*/
	void GraphicsScene::Initialize()
	{
		/// [EN] The scene renderer shares the device; its Upload ring holds 64 MB per frame, enough for the first frame of a large scene, when every Model table row and the full bone palette go up at once.
		/// [JP] SceneRenderer はデバイスを共有する。Upload のリングはフレームあたり 64 MB。大きなシーンの最初のフレームで、Model の表の全行とボーンのパレット全体を一度に送っても足りる大きさ。
		renderer_ = MakePtr<SceneRenderer>(device_);
		renderer_->Initialize(RenderFeatureSet::Scene, 64ull * 1024 * 1024);

		screenView_ = device_.RenderTargetHeap().AllocateIndex();
	}

	/**
	* [EN]
	* Releases the views' output textures, the screen's render target view slot and the scene renderer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各ビューの出力テクスチャ、画面の RTV の枠、SceneRenderer を解放する。
	*/
	void GraphicsScene::Finalize()
	{
		for (RenderGraphAllocation& allocation : outputs_)
		{
			RenderGraphMemory::Release(device_, allocation);
		}

		/// [EN] The back buffer belongs to the swap chain, so only the slot goes back to the heap.
		/// [JP] バックバッファの持ち主はスワップチェインなので、ヒープへは枠だけを返す。
		device_.RenderTargetHeap().Release(nullptr, screenView_);
		screenView_ = SC_INVALID;

		renderer_.reset();
	}

	/**
	* [EN]
	* Records the output size and the render size the upscale mode gives for it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 出力のサイズと、アップスケールのモードから決まる描画のサイズを記録する。
	*/
	void GraphicsScene::Configure(const GraphicsSettings& settings)
	{
		/// [EN] Clamp the scaled render extent to a valid minimum render size.
		/// [JP] 倍率適用後の描画サイズを、有効な最小サイズへクランプする。
		Float scale = UpscaleRenderScale(settings.upscale_.mode_);
		nativeWidth_ = Max<Uint32>(64, static_cast<Uint32>(settings.outputWidth_ * scale + 0.5f));
		nativeHeight_ = Max<Uint32>(64, static_cast<Uint32>(settings.outputHeight_ * scale + 0.5f));
		outputWidth_ = settings.outputWidth_;
		outputHeight_ = settings.outputHeight_;
		upscale_ = settings.upscale_;
	}

	/**
	* [EN]
	* Empties the collected views and allows one render again.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 集めたビューを空にし、再び 1 回描画できるようにする。
	*/
	void GraphicsScene::Begin()
	{
		/// [EN] The collected views and the render both belong to one presentation frame.
		/// [JP] 集めたビューと描画は、どちらも 1 つの表示フレームに属する。
		rendered_ = false;
		viewCount_ = 0;
		world_ = nullptr;
	}

	/**
	* [EN]
	* Builds the editor view from the editor camera and adds it to this frame's views.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディターカメラからエディタービューを組み立て、このフレームのビューに足す。
	*/
	void GraphicsScene::AddEditorView(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode, RenderDebugSource debugSource, Uint64 showFlags)
	{
		if (viewCount_ >= RenderViewCount)
		{
			return;
		}

		RenderView& view = views_[viewCount_];
		++viewCount_;
		world_ = &world;

		/// [EN] The view renders at the scaled size and is displayed at the output size.
		/// [JP] ビューは倍率適用後のサイズで描き、出力のサイズで表示する。
		view.kind_ = RenderViewKind::Editor;
		view.mode_ = viewMode;
		view.debugSource_ = debugSource;
		view.showFlags_ = showFlags;
		view.output_ = RenderViewOutput::Texture;
		view.renderWidth_ = nativeWidth_;
		view.renderHeight_ = nativeHeight_;
		view.displayWidth_ = outputWidth_;
		view.displayHeight_ = outputHeight_;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(outputWidth_), static_cast<Float>(outputHeight_));
		view.hasActiveCamera_ = true;

		/// [EN] Current and previous transforms let temporal passes compare adjacent frames.
		/// [JP] 現在と前回の変換行列により、テンポラルなパスが隣接フレームを比較できる。
		SceneConstantBuffer& scene = view.scene_;
		scene = SceneConstantBuffer{};
		scene.view_ = editorCamera.View();
		scene.inverseView_ = editorCamera.InverseView();
		scene.projection_ = editorCamera.Projection();
		scene.inverseProjection_ = editorCamera.InverseProjection();
		scene.nonJitterProjection_ = editorCamera.NonJitterProjection();
		scene.currentViewProjection_ = editorCamera.CurrentViewProjection();
		scene.previousViewProjection_ = editorCamera.PreviousViewProjection();
		scene.inverseViewProjection_ = editorCamera.InverseViewProjection();
		scene.nonJitterViewProjection_ = editorCamera.NonJitterViewProjection();
		scene.previousNonJitterViewProjection_ = editorCamera.PreviousNonJitterViewProjection();

		/// [EN] Camera pose, projection range, debug view mode, frame timing and the render and display sizes.
		/// [JP] カメラ姿勢、投影範囲、デバッグビュー、フレーム時刻、描画と表示のサイズ。
		scene.cameraPosition_ = Vector4(editorCamera.Eye().x, editorCamera.Eye().y, editorCamera.Eye().z, 1.0f);
		scene.cameraFocus_ = Vector4(editorCamera.Focus().x, editorCamera.Focus().y, editorCamera.Focus().z, 1.0f);
		scene.fieldOfView_ = editorCamera.Fov();
		scene.nearPlane_ = editorCamera.Near();
		scene.farPlane_ = editorCamera.Far();
		scene.viewMode_ = static_cast<Uint>(viewMode);
		scene.showFlagsLow_ = static_cast<Uint>(showFlags);
		scene.showFlagsHigh_ = static_cast<Uint>(showFlags >> 32);
		scene.debugSource_ = static_cast<Uint>(debugSource);
		scene.totalTime_ = timer.TotalTime();
		scene.deltaTime_ = timer.DeltaTime();
		scene.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		scene.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		scene.displaySize_ = Vector2(static_cast<Float>(outputWidth_), static_cast<Float>(outputHeight_));
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
	void GraphicsScene::AddGameView(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect)
	{
		/// [EN] CameraSystem resolves the active game camera for the current world and render extent.
		/// [JP] CameraSystem は、現在のワールドと描画サイズに対するアクティブゲームカメラを解決する。
		cameraSystem.Update(world, timer, static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));

		if (viewCount_ >= RenderViewCount)
		{
			return;
		}

		RenderView& view = views_[viewCount_];
		++viewCount_;
		world_ = &world;

		view.kind_ = RenderViewKind::Game;
		view.mode_ = RenderViewMode::Lit;
		view.debugSource_ = RenderDebugSource::Reflection;
		view.showFlags_ = static_cast<Uint64>(RenderShowFlag::All);
		view.output_ = output;
		view.renderWidth_ = nativeWidth_;
		view.renderHeight_ = nativeHeight_;
		view.displayWidth_ = outputWidth_;
		view.displayHeight_ = outputHeight_;
		view.outputRect_ = outputRect;
		view.hasActiveCamera_ = cameraSystem.ActiveCamera();

		/// [EN] The camera system's constants are complete except for the display size, which the scene knows.
		/// [JP] カメラシステムの定数は、シーンが知っている表示サイズ以外はそろっている。
		view.scene_ = cameraSystem.GetSceneConstantBuffer();
		view.scene_.displaySize_ = Vector2(static_cast<Float>(outputWidth_), static_cast<Float>(outputHeight_));
		view.scene_.viewMode_ = static_cast<Uint>(RenderViewMode::Lit);
		view.scene_.showFlagsLow_ = static_cast<Uint>(view.showFlags_);
		view.scene_.showFlagsHigh_ = static_cast<Uint>(view.showFlags_ >> 32);
		view.scene_.debugSource_ = static_cast<Uint>(view.debugSource_);
	}

	/**
	* [EN]
	* Builds the canvas view from the canvas camera and adds it to this frame's views.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャンバスカメラからキャンバスビューを組み立て、このフレームのビューに足す。
	*/
	void GraphicsScene::AddCanvasView(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world)
	{
		if (viewCount_ >= RenderViewCount)
		{
			return;
		}

		RenderView& view = views_[viewCount_];
		++viewCount_;
		world_ = &world;

		/// [EN] Canvas output is native-sized, so its display size equals its render size.
		/// [JP] キャンバスの出力はネイティブサイズのため、表示サイズは描画サイズと等しい。
		view.kind_ = RenderViewKind::Canvas;
		view.mode_ = RenderViewMode::Lit;
		view.debugSource_ = RenderDebugSource::Reflection;
		view.showFlags_ = static_cast<Uint64>(RenderShowFlag::All);
		view.output_ = RenderViewOutput::Texture;
		view.renderWidth_ = nativeWidth_;
		view.renderHeight_ = nativeHeight_;
		view.displayWidth_ = nativeWidth_;
		view.displayHeight_ = nativeHeight_;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		view.hasActiveCamera_ = true;

		/// [EN] The canvas camera's transforms and temporal history.
		/// [JP] キャンバスカメラの変換行列とテンポラルの履歴。
		SceneConstantBuffer& scene = view.scene_;
		scene = SceneConstantBuffer{};
		scene.view_ = canvasCamera.View();
		scene.inverseView_ = canvasCamera.InverseView();
		scene.projection_ = canvasCamera.Projection();
		scene.inverseProjection_ = canvasCamera.InverseProjection();
		scene.nonJitterProjection_ = canvasCamera.NonJitterProjection();
		scene.currentViewProjection_ = canvasCamera.CurrentViewProjection();
		scene.previousViewProjection_ = canvasCamera.PreviousViewProjection();
		scene.inverseViewProjection_ = canvasCamera.InverseViewProjection();
		scene.nonJitterViewProjection_ = canvasCamera.NonJitterViewProjection();

		/// [EN] The canvas camera's pose, projection range, frame timing and sizes.
		/// [JP] キャンバスカメラの姿勢、投影範囲、フレーム時刻、サイズ。
		scene.cameraPosition_ = Vector4(canvasCamera.Eye().x, canvasCamera.Eye().y, canvasCamera.Eye().z, 1.0f);
		scene.cameraFocus_ = Vector4(canvasCamera.Focus().x, canvasCamera.Focus().y, canvasCamera.Focus().z, 1.0f);
		scene.fieldOfView_ = canvasCamera.Fov();
		scene.nearPlane_ = canvasCamera.Near();
		scene.farPlane_ = canvasCamera.Far();
		scene.totalTime_ = timer.TotalTime();
		scene.deltaTime_ = timer.DeltaTime();
		scene.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		scene.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		scene.displaySize_ = scene.screenSize_;
		scene.showFlagsLow_ = static_cast<Uint>(view.showFlags_);
		scene.showFlagsHigh_ = static_cast<Uint>(view.showFlags_ >> 32);
		scene.debugSource_ = static_cast<Uint>(view.debugSource_);
	}

	/**
	* [EN]
	* Renders every view collected this frame through SceneRenderer, once per frame, into each view's output texture or into screen, with celestial as the frame's sun and moon.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このフレームに集めた全ビューを、SceneRenderer でフレームに 1 回、各ビューの出力テクスチャか screen へ描画する。celestial はフレームの太陽と月。
	*/
	void GraphicsScene::Render(ID3D12Resource* screen, const CelestialResult& celestial, const ShapeList& shapes)
	{
		if (rendered_ || viewCount_ == 0 || !world_)
		{
			return;
		}
		rendered_ = true;

		Benchmark::Begin("Render");

		D3D12CommandList& cmdList = *device_.Context().GetDirectList();
		DynamicArray<RenderGraphImport> imports;
		for (Uint32 viewIndex = 0; viewIndex < viewCount_; ++viewIndex)
		{
			const RenderView& view = views_[viewIndex];

			/// [EN] A view shown on the screen writes straight into the back buffer: its view is rewritten into the slot every frame (a render target view is read when the target is bound, so one slot is enough), and the graph returns it in the render-target state the presenter left it in.
			/// [JP] 画面に出すビューは、バックバッファへ直接書く。ビューは毎フレーム枠に書き直し（RTV は描き込み先を結び付けた時点で読まれるので、枠は 1 つで足りる）、グラフは表示器が置いた render-target 状態に戻して返す。
			if (view.output_ == RenderViewOutput::Screen)
			{
				D3D12_RESOURCE_DESC screenDesc = screen->GetDesc();
				device_.Context().GetDevice()->CreateRenderTargetView(screen, nullptr, device_.RenderTargetHeap().CPUHandle(screenView_));

				RenderGraphAllocation screenOutput;
				screenOutput.resource_ = screen;
				screenOutput.state_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
				screenOutput.format_ = static_cast<RenderFormat>(screenDesc.Format);
				screenOutput.width_ = static_cast<Uint32>(screenDesc.Width);
				screenOutput.height_ = screenDesc.Height;
				screenOutput.renderTarget_ = true;
				screenOutput.renderTargetView_ = screenView_;
				imports.push_back(RenderGraphImport{ String(HUDFeature::ViewOutput.Name()), view.kind_, screenOutput });
				continue;
			}

			/// [EN] Any other view gets an output texture of its display size, imported as ViewOutput. A new texture is cleared to black and left in the pixel-shader-resource state, which the graph returns it to after every frame, so the editor UI can read it.
			/// [JP] それ以外のビューには、表示の大きさの出力テクスチャを用意し、ViewOutput として取り込む。新しく作ったテクスチャは黒で塗ってピクセルシェーダーリソースの状態にしておく。グラフは毎フレームこの状態に戻して返すので、エディター UI がそのまま読める。
			RenderGraphAllocation& output = outputs_[static_cast<Size>(view.kind_)];
			if (!output.resource_ || output.width_ != view.displayWidth_ || output.height_ != view.displayHeight_)
			{
				RenderGraphMemory::Release(device_, output);

				RenderGraphResource resource;
				resource.resourceName_ = String(HUDFeature::ViewOutput.Name());
				resource.desc_.shape_ = RenderResourceShape::Texture2D;
				resource.desc_.scale_ = RenderResourceScale::Fixed;
				resource.desc_.format_ = RenderFormat::RGBA8Unorm;
				resource.shaderResource_ = true;
				resource.unorderedAccess_ = true;
				resource.renderTarget_ = true;
				RenderGraphInstance instance;
				instance.width_ = view.displayWidth_;
				instance.height_ = view.displayHeight_;
				output = RenderGraphMemory::Create(device_, resource, instance, { 0.0f, 0.0f, 0.0f, 1.0f });

				StaticArray<Float, 4> black = { 0.0f, 0.0f, 0.0f, 1.0f };
				cmdList.Barrier(output.resource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
				cmdList.Get()->ClearRenderTargetView(device_.RenderTargetHeap().CPUHandle(output.renderTargetView_), black.data(), 0, nullptr);
				cmdList.Barrier(output.resource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				output.state_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			imports.push_back(RenderGraphImport{ String(HUDFeature::ViewOutput.Name()), view.kind_, output });
			display_.views_[static_cast<Size>(view.kind_)] = device_.ResourceHeap().GPUHandle(output.shaderResourceView_);
		}

		/// [EN] The scene is everything this frame draws: the World, the asset resources, the sun and moon, the upscale settings, the shapes and colliders, the collected views and their outputs.
		/// [JP] シーンは、このフレームに描くものすべて。World、アセットのリソース、太陽と月、アップスケールの設定、形とコライダー、集めたビューとその出力。
		RenderScene scene;
		scene.world_ = world_;
		scene.resource_ = &module_.Resources();
		scene.Set(celestial);
		scene.Set(upscale_);
		scene.Set(shapes);
		scene.views_ = std::span<const RenderView>(views_.data(), viewCount_);
		scene.imports_ = imports;
		renderer_->Render(scene);

		Benchmark::End("Render");
	}

	/**
	* [EN]
	* Returns the aspect ratio of the render size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 描画のサイズの縦横比を返す。
	*/
	Float GraphicsScene::AspectRatio()const
	{
		return static_cast<Float>(nativeWidth_) / static_cast<Float>(nativeHeight_);
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
	const GpuProfiler& GraphicsScene::Profiler()const
	{
		return renderer_->Profiler();
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
	const GraphicsDisplay& GraphicsScene::Display()const
	{
		return display_;
	}
}
