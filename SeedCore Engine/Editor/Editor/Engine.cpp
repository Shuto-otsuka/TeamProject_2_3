#include <Editor/Editor/Engine.h>
#include <FoundationEngine/Utility/Bootstrap.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Coroutine/CoroutineSystem.h>
#include <FoundationEngine/File/FileDirectory.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <AudioEngine/Audio/AudioSystem.h>
#include <AudioEngine/Audio/Audio.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>
#include <FoundationEngine/Interop/QueryInstance.h>
#include <FoundationEngine/Interop/RenderInstance.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/Avatar/AvatarMesh.h>
#include <GraphicsEngine/Renderer/BootScreenRenderer.h>
#include <GraphicsEngine/D3D12/Context/D3D12Adapter.h>

#include <Editor/Editor/Build/AtomCraft.h>
#include <FoundationEngine/Application/Application.h>


namespace SeedCore
{
	void Engine::Boot(const Bootstrap& boot)
	{
		if (!BootWindow(boot))
		{
			return;
		}

		world_ = MakePtr<World>();
		gameTimer_ = world_->AddService<GameTimer>();

		system_ = MakePtr<SystemScheduler>();

		auto worker = MakeWorkerInterface<JobScheduler>();
		executor_ = MakePtr<JobExecutor>(std::thread::hardware_concurrency(), std::move(worker));

		if (!BootPhysics())
		{
			return;
		}

		if (!BootGraphics())
		{
			return;
		}

		editorConfig_.Load();

		if (!BootAudio())
		{
			return;
		}

		if (!BootFont())
		{
			return;
		}

		if (!BootImGui())
		{
			return;
		}

		audioModule_ = MakePtr<AudioModule>();
		resource_ = MakePtr<ResourceCache>();
		graphics_->GetGraphicsModule().Install(*resource_);
		audioModule_->Install(*resource_);
		resource_->Async();

		QueryInstance* queryInstance = world_->AddService<QueryInstance>();
		RenderInstance* renderInstance = world_->AddService<RenderInstance>();
		world_->AddService<Physics>(*queryInstance);
		world_->AddService<DebugDraw>(*renderInstance);
		world_->AddService<Audio>();
		world_->AddService<Application>();

		sceneSystem_ = MakePtr<SceneSystem>(*world_, *resource_, *executor_);
		prefabSystem_ = MakePtr<PrefabSystem>(*world_, *resource_);

		editorContext_.world_.world_ = world_.get();
		editorContext_.world_.resource_ = resource_.get();
		editorContext_.world_.sceneSystem_ = sceneSystem_.get();
		editorContext_.world_.timer_ = gameTimer_;
		editorContext_.world_.system_ = system_.get();
		editorContext_.view_.editor_.camera_ = &editorCamera_;
		editorContext_.view_.editor_.cameraController_ = &editorCameraController_;
		editorContext_.view_.canvas_.camera_ = &canvasCamera_;
		editorContext_.preview_.timeline_.camera_ = &timelineCamera_;
		editorContext_.preview_.timeline_.cameraController_ = &timelineCameraController_;
		editorContext_.preview_.modelTransform_.camera_ = &modelTransformCamera_;
		editorContext_.preview_.modelTransform_.cameraController_ = &modelTransformCameraController_;
		editorContext_.preview_.material_.camera_ = &materialCamera_;
		editorContext_.preview_.material_.cameraController_ = &materialCameraController_;
		editorContext_.preview_.skeletonController_.camera_ = &skeletonControllerCamera_;
		editorContext_.preview_.skeletonController_.cameraController_ = &skeletonControllerCameraController_;
		editorContext_.preview_.avatar_.camera_ = &avatarCamera_;
		editorContext_.preview_.avatar_.cameraController_ = &avatarCameraController_;
		editorContext_.graphics_.graphics_ = graphics_.get();
		editorContext_.graphics_.imgui_ = imgui_.get();

		InputSystem::Initialize();
		LayerRegistry::Load();

		csharpHost_.Initialize(FileDirectory::ExecutableDirectory());
		csharpHost_.Load(*world_);
		pluginHost_.Initialize(FileDirectory::ExecutableDirectory(), ImGui::GetCurrentContext());
		pluginHost_.Load(*world_);
		hotReload_.Initialize(pluginHost_, csharpHost_);

		editorCamera_.Eye(editorConfig_.cameraEye_);
		editorCamera_.Focus(editorConfig_.cameraFocus_);
		editorCamera_.Up(editorConfig_.cameraUp_);
		editorCamera_.Fov(editorConfig_.cameraFov_);

		editorCameraController_.MoveSpeed(editorConfig_.cameraMoveSpeed_);
		editorCameraController_.RotateSpeed(editorConfig_.cameraRotateSpeed_);
		editorCameraController_.ScrollSpeed(editorConfig_.cameraScrollSpeed_);
		editorCameraController_.PanSpeed(editorConfig_.cameraPanSpeed_);
		editorCameraController_.ShiftSpeedMultiplier(editorConfig_.cameraShiftSpeedMultiplier_);

		imgui_->FontScale(editorConfig_.fontScale_);

		std::error_code atomCraftErrorCode;
		if (editorConfig_.atomCraftPath_.view().empty() || !std::filesystem::exists(editorConfig_.atomCraftPath_.c_str(), atomCraftErrorCode))
		{
			editorConfig_.atomCraftPath_ = AtomCraft::Detect();

			if (editorConfig_.atomCraftPath_.view().empty())
			{
				SC_LOG_WARNING("CRI Atom Craft が見つかりませんでした。オーディオのビルドには ADX LE ツールが必要です: https://game.criware.jp/products/adx-le/");
			}
			else
			{
				editorConfig_.Save();
			}
		}

		/// [EN] The game config is loaded once here and, with the editor config loaded above, shared with the config panel through the context; the render targets are sized from it on the first frame.
		/// [JP] ゲームの設定はここで1回だけ読み込み、先に読み込んだエディタの設定と一緒にコンテキスト経由で設定パネルと共有する。描画先の大きさは最初のフレームでこれから決める。
		gameConfig_.Load();
		editorContext_.config_.game_ = &gameConfig_;
		editorContext_.config_.editor_ = &editorConfig_;
		editorContext_.config_.resizeRequested_ = true;

		graphics_->Reflex(gameConfig_.useReflex_, false);
		graphics_->DeepDVC(gameConfig_.useDeepDVC_, 0.5f, 0.25f);
		graphics_->FrameGeneration(gameConfig_.useFrameGeneration_);

		if (!editorConfig_.lastScenePath_.str().empty())
		{
			std::filesystem::path lastScenePath = editorConfig_.lastScenePath_.str();
			SceneVisual visual;
			if (sceneSystem_->Load(lastScenePath, &visual))
			{
				editorContext_.sceneVisual_.raytracing_ = DeserializeRaytracingContext(visual.raytracing_);
				editorContext_.sceneVisual_.screenSpace_ = DeserializeScreenSpaceContext(visual.screenSpace_);
				editorContext_.sceneVisual_.rasterization_ = DeserializeRasterizationContext(visual.rasterization_);
				editorContext_.sceneVisual_.qualityPreset_ = GraphicsQualityPreset::Custom;
				editorContext_.scene_.path_ = FilePath(lastScenePath, resource_->ProjectRootPath());
				editorContext_.config_.resizeRequested_ = true;
			}
		}

		editor_ = MakePtr<Editor>(editorContext_, cameraSystem_);
	}

	void Engine::Shutdown()
	{
		if (graphics_)
		{
			graphics_->Wait();
		}

		if (imgui_)
		{
			editorConfig_.cameraEye_ = editorCamera_.Eye();
			editorConfig_.cameraFocus_ = editorCamera_.Focus();
			editorConfig_.cameraUp_ = editorCamera_.Up();
			editorConfig_.cameraFov_ = editorCamera_.Fov();

			editorConfig_.cameraMoveSpeed_ = editorCameraController_.MoveSpeed();
			editorConfig_.cameraRotateSpeed_ = editorCameraController_.RotateSpeed();
			editorConfig_.cameraScrollSpeed_ = editorCameraController_.ScrollSpeed();
			editorConfig_.cameraPanSpeed_ = editorCameraController_.PanSpeed();
			editorConfig_.cameraShiftSpeedMultiplier_ = editorCameraController_.ShiftSpeedMultiplier();

			editorConfig_.fontScale_ = imgui_->FontScale();
			editorConfig_.lastScenePath_ = String(editorContext_.scene_.path_.FullPath().string());

			editorConfig_.Save();
		}

		if (window_)
		{
			window_->RestoreAccessibility();
		}

		if (world_)
		{
			pluginHost_.Unload(*world_);
			csharpHost_.Unload(*world_);
		}

		InputSystem::Finalize();

		if (editor_)
		{
			editor_.reset();
			editor_ = nullptr;
		}

		if (sceneSystem_)
		{
			sceneSystem_.reset();
			sceneSystem_ = nullptr;
		}

		if (prefabSystem_)
		{
			prefabSystem_.reset();
			prefabSystem_ = nullptr;
		}

		if (resource_)
		{
			resource_.reset();
			resource_ = nullptr;
		}

		if (imgui_)
		{
			imgui_->Finalize();
			imgui_.reset();
			imgui_ = nullptr;
		}

		if (audioModule_)
		{
			audioModule_.reset();
			audioModule_ = nullptr;
		}
		
		if (fontManager_)
		{
			fontManager_->Finalize();
			fontManager_.reset();
			fontManager_ = nullptr;
		}

		/// [EN] The world is torn down while the physics world still exists, since components such as CharacterController release Jolt objects that reach back into it; its audio module is already gone, so the world's audio service is destroyed first.
		/// [JP] CharacterController などのコンポーネントは解放時に物理ワールドへ触れる Jolt のオブジェクトを手放すため、World は物理ワールドがまだある間に破棄する。オーディオモジュールは既に無いので、World のオーディオのサービスを先に壊す。
		if (world_)
		{
			world_->RemoveService<Audio>();
			world_.reset();
			world_ = nullptr;
		}

		if (joltManager_)
		{
			joltManager_->Finalize();
			joltManager_.reset();
			joltManager_ = nullptr;
		}

		if (criManager_)
		{
			criManager_->Finalize();
			criManager_.reset();
			criManager_ = nullptr;
		}

		if (graphics_)
		{
			graphics_->Finalize();
			graphics_.reset();
			graphics_ = nullptr;
		}
	}

	void Engine::MainLoop()
	{
		MSG msg{};
		while (WM_QUIT != msg.message) [[likely]]
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) [[unlikely]]
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				if (window_->ConsumeCloseRequested())
				{
					editorContext_.application_.exitRequested_ = true;
				}

				window_->CalculateFrameStats();
				window_->GetTimer().Tick();
				worldTimer_.Tick(window_->GetTimer().Delta());
				gameTimer_->Tick(window_->GetTimer().Delta());

				/// [EN] The game rect is the one the game view published last frame; it only changes when the view is resized or moved.
				/// [JP] ゲームの矩形は、ゲームビューが前のフレームに公開したもの。変わるのはビューの大きさや位置を変えたときだけ。
				InputSystem::Update(editor_->GameViewImageHovered(), cameraSystem_.Rect());

				hotReload_.Tick(*world_);
				pluginHost_.Tick(*world_);

				editorContext_.application_.uiFrame_++;

				if (editorContext_.config_.resizeRequested_)
				{
					editorContext_.config_.resizeRequested_ = false;

					ScResolution::ResSize outputSize = ToResSize(gameConfig_.resolution_);
					Uint32 outputWidth = static_cast<Uint32>(outputSize.Width);
					Uint32 outputHeight = static_cast<Uint32>(outputSize.Height);

					graphics_->ResizeRenderTarget(outputWidth, outputHeight, gameConfig_.upscaleMode_);
				}

				if (editorContext_.config_.recreateRequested_)
				{
					editorContext_.config_.recreateRequested_ = false;

					graphics_->FrameGeneration(gameConfig_.useFrameGeneration_);
				}

				splashSystem_.Update(*resource_);
				if (!splashSystem_.Complete())
				{
					resource_->StepAsync(graphics_->GetContext().GetDevice(), graphics_->GetContext().GetDirectQueue(), graphics_->GetBC7CompressShader());
					graphics_->DrawSplashScreen(splashSystem_);
					continue;
				}

				graphics_->Begin();

				AudioSystem::ResolveSound(audioModule_->Resource(), *world_);

				if (gameTimer_->Playing())
				{
					/// [EN] Coroutines resume before physics and the script ticks, with this frame's input already read; they do not advance while paused, so frame waits do not count down.
					/// [JP] コルーチンは物理とスクリプトの Tick より前に、今フレームの入力を読んだ後で再開する。ポーズ中は進めないので、フレーム待ちも減らない。
					if (!gameTimer_->Paused())
					{
						CoroutineSystem::Update(gameTimer_->ScaledDeltaTime());
					}

					PhysicsSystem::ResolveMeshCollider(*graphics_->GetGraphicsModule().Resources().meshCollisionResource_, *world_);
					PhysicsSystem::ResolveSoftbody(*graphics_->GetGraphicsModule().Resources().modelResource_, *world_);
					PhysicsSystem::ApplyActive(*world_);
					PhysicsSystem::ApplyCollider(*world_);

					joltManager_->ActiveWorld(world_.get());

					while (gameTimer_->Step())
					{
						PhysicsSystem::ApplyRigidbody(*world_, gameTimer_->FixedDeltaTime());
						joltManager_->Execute(gameTimer_->FixedDeltaTime());
						system_->Step(*world_, gameTimer_->FixedDeltaTime());
					}
				}

				system_->Run(*world_, *resource_, *executor_, worldTimer_.DeltaTime(), gameTimer_->ScaledDeltaTime(), gameTimer_->Playing());

				if (gameTimer_->Playing())
				{
					sceneSystem_->Update(gameTimer_->ScaledDeltaTime());

					if (const SceneBlueprint* switchedScene = sceneSystem_->ConsumeSwitchedScene())
					{
						editorContext_.sceneVisual_.raytracing_ = DeserializeRaytracingContext(switchedScene->Visual().raytracing_);
						editorContext_.sceneVisual_.screenSpace_ = DeserializeScreenSpaceContext(switchedScene->Visual().screenSpace_);
						editorContext_.sceneVisual_.rasterization_ = DeserializeRasterizationContext(switchedScene->Visual().rasterization_);
					}
				}

				if (gameTimer_->Playing())
				{
					AudioSystem::Update(*world_, gameTimer_->ScaledDeltaTime());
				}

				criManager_->Execute();

				imgui_->NewFrame();

				editorCamera_.Tick(window_->GetTimer().Delta());

				canvasCamera_.Resize(ScResolution::SC_CANVAS.Width, ScResolution::SC_CANVAS.Height);
				canvasCamera_.Tick(window_->GetTimer().Delta());
				timelineCamera_.Tick(window_->GetTimer().Delta());
				modelTransformCamera_.Tick(window_->GetTimer().Delta());
				materialCamera_.Tick(window_->GetTimer().Delta());
				skeletonControllerCamera_.Tick(window_->GetTimer().Delta());
				avatarCamera_.Tick(window_->GetTimer().Delta());

				if (editorContext_.sceneVisual_.raytracing_.daySystemEnabled_)
				{
					CelestialSystem::Advance(gameTimer_->ScaledDeltaTime(), editorContext_.sceneVisual_.raytracing_.daySystem_);

					if (editorContext_.sceneVisual_.raytracing_.sunLightEnabled_)
					{
						CelestialResult celestial = CelestialSystem::Compute(editorContext_.sceneVisual_.raytracing_.daySystem_, editorContext_.sceneVisual_.raytracing_.sunLight_, editorContext_.sceneVisual_.raytracing_.moonLight_);
						for (Int index = 0; index < 3; index++)
						{
							editorContext_.sceneVisual_.raytracing_.volumetricCloudScapes_.skyZenithColor_[index] = celestial.skyZenithColor_[index];
							editorContext_.sceneVisual_.raytracing_.volumetricCloudScapes_.skyHorizonColor_[index] = celestial.skyHorizonColor_[index];
						}
					}
				}
				weatherSystem_.Execute(*world_, gameTimer_->ScaledDeltaTime(), editorContext_.sceneVisual_.raytracing_.daySystem_.monthOfYear_, editorContext_.sceneVisual_.raytracing_.volumetricCloudScapes_);

				graphics_->Raytracing(editor_->GetRaytracingSettings());
				graphics_->Upscale(gameConfig_.useDlss_, gameConfig_.upscaleMode_);
				graphics_->VerticalSync(gameConfig_.vsync_);
				graphics_->ShapeVisible(editorContext_.view_.editor_.shapeVisible_);

				graphics_->EditorRender(worldTimer_, editorCamera_, *world_, editorContext_.view_.editor_.viewMode_, editor_->GetSelectedEntities());
				graphics_->GameRender(*gameTimer_, cameraSystem_, *world_);
				graphics_->CanvasRender(worldTimer_, canvasCamera_, *world_);

				const PreviewContext& preview = editorContext_.preview_;

				if (preview.timeline_.active_)
				{
					graphics_->TimelineRender(worldTimer_, timelineCamera_, preview.timeline_.meshAssetID_, preview.timeline_.animationAssetID_, preview.timeline_.time_, Matrix::Identity);
				}

				if (preview.modelTransform_.active_)
				{
					graphics_->ModelTransformRender(worldTimer_, modelTransformCamera_, preview.modelTransform_.meshAssetID_, 0, 0.0f, preview.modelTransform_.worldMatrix_);
				}

				if (preview.material_.active_)
				{
					graphics_->MaterialRender(worldTimer_, materialCamera_, preview.material_.meshAssetID_, preview.material_.surfaceAssetID_, Matrix::Identity);
				}

				if (preview.skeletonController_.active_)
				{
					graphics_->SkeletonControllerRender(worldTimer_, skeletonControllerCamera_, preview.skeletonController_.meshAssetID_, 0, 0.0f, Matrix::Identity, preview.skeletonController_.nodeIndex_);
				}

				if (preview.avatar_.active_ && preview.avatar_.mesh_)
				{
					preview.avatar_.mesh_->Update(preview.avatar_.positions_, preview.avatar_.normals_);
					graphics_->AvatarRender(worldTimer_, avatarCamera_, *preview.avatar_.mesh_, preview.avatar_.boneCount_, Matrix::Identity, std::span<const Uint32>(preview.avatar_.regionTextureIndices_, preview.avatar_.regionCount_));
				}

				if (preview.bootScreen_.active_ && preview.bootScreen_.renderer_ && preview.bootScreen_.config_)
				{
					preview.bootScreen_.renderer_->Render(graphics_->GetContext().GetDirectList(), *preview.bootScreen_.config_, preview.bootScreen_.progress_, worldTimer_.TotalTime());
				}

				graphics_->Bind();

				imgui_->DockSpaceBegin(editor_->DrawToolbar());
				editor_->Draw(graphics_->EditorDisplayGPUHandle(), graphics_->GameDisplayGPUHandle(), graphics_->CanvasDisplayGPUHandle(), graphics_->TimelineDisplayGPUHandle(), graphics_->ModelTransformDisplayGPUHandle(), graphics_->MaterialDisplayGPUHandle(), graphics_->SkeletonControllerDisplayGPUHandle(), graphics_->AvatarDisplayGPUHandle(), graphics_->GetGpuProfiler());
				imgui_->DockSpaceEnd();

				imgui_->Render(graphics_->GetContext().GetDirectList()->Get());

				graphics_->End();
				graphics_->GetSwapChain().Present(graphics_->GetContext().GetDevice());
			}
		}
	}

	Bool Engine::BootWindow(const Bootstrap& boot)
	{
		window_ = MakePtr<Window>();
		if (window_)
		{
			hwnd_ = window_->Create(boot);
			window_->GetTimer().Start();
			return true;
		}
		return false;
	}

	Bool Engine::BootPhysics()
	{
		joltManager_ = MakePtr<JoltManager>();
		if (!joltManager_->Initialize(*executor_))
		{
			return false;
		}
		Gateway::BindJoltManager(joltManager_.get());
		return true;
	}

	Bool Engine::BootGraphics()
	{
		splashSystem_.Initialize(false, false);

		RECT clientRect{};
		GetClientRect(hwnd_, &clientRect);
		Uint32 clientWidth = static_cast<Uint32>(clientRect.right - clientRect.left);
		Uint32 clientHeight = static_cast<Uint32>(clientRect.bottom - clientRect.top);

		graphics_ = MakePtr<Graphics>();
		return graphics_->Initialize(hwnd_, clientWidth, clientHeight, splashSystem_.Config());
	}

	Bool Engine::BootAudio()
	{
		criManager_ = MakePtr<CriManager>(editorConfig_.acfPath_);
		if (!criManager_->Initialize())
		{
			return false;
		}
		Gateway::BindCriManager(criManager_.get());
		return true;
	}

	Bool Engine::BootFont()
	{
		fontManager_ = MakePtr<FontManager>();
		if (!fontManager_->Initialize())
		{
			return false;
		}
		Gateway::BindFontManager(fontManager_.get());
		return true;
	}

	Bool Engine::BootImGui()
	{
		imgui_ = MakePtr<ImGuiRenderer>();
		return imgui_->Initialize(hwnd_, graphics_->GetContext().GetDevice(), graphics_->GetContext().GetDirectQueue()->GetCommandQueue(), &graphics_->GetBindlessHeap(), static_cast<Int>(graphics_->GetSwapChain().BufferCount()));
	}
}
