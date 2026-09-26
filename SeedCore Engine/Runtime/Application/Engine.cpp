#include <Runtime/Application/Engine.h>
#include <FoundationEngine/Utility/Bootstrap.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Coroutine/CoroutineSystem.h>
#include <FoundationEngine/File/FileDirectory.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <AudioEngine/Audio/AudioSystem.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/Log/Error.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/System/CelestialSystem.h>

namespace SeedCore
{
	void Engine::Boot(const Bootstrap& boot)
	{
		gameConfig_.Load();

		Bootstrap windowBoot = boot;
		windowBoot.WindowDesc_.Width_ = gameConfig_.windowWidth_;
		windowBoot.WindowDesc_.Height_ = gameConfig_.windowHeight_;
		windowBoot.WindowDesc_.Fullscreen_ = gameConfig_.fullscreen_;

		std::wstring windowTitle = gameConfig_.executableName_.w_str();
		if (!windowTitle.empty())
		{
			windowBoot.WindowDesc_.Title_ = windowTitle.c_str();
		}

		if (!BootWindow(windowBoot))
		{
			return;
		}

		world_ = MakePtr<World>();
		world_->Timer(gameTimer_);

		system_ = MakePtr<SystemScheduler>();

		auto worker = MakeWorkerInterface<JobScheduler>();
		executor_ = MakePtr<JobExecutor>(std::thread::hardware_concurrency(), std::move(worker));

		joltManager_ = MakePtr<JoltManager>();
		if (!joltManager_->Initialize(*executor_))
		{
			return;
		}
		Gateway::BindJoltManager(joltManager_.get());

		RECT clientRect{};
		GetClientRect(hwnd_, &clientRect);
		Uint32 clientWidth = static_cast<Uint32>(clientRect.right - clientRect.left);
		Uint32 clientHeight = static_cast<Uint32>(clientRect.bottom - clientRect.top);

		graphics_ = MakePtr<Graphics>();
		if (!graphics_->Initialize(hwnd_, static_cast<Float>(clientWidth), static_cast<Float>(clientHeight)))
		{
			return;
		}

		Uint32 resizedWidth = 0;
		Uint32 resizedHeight = 0;
		window_->ConsumeResized(resizedWidth, resizedHeight);

		ID3D12Device* device = graphics_->GetContext()->GetDevice();

		criManager_ = MakePtr<CriManager>();
		if (!criManager_->Initialize())
		{
			return;
		}
		Gateway::BindCriManager(criManager_.get());

		fontManager_ = MakePtr<FontManager>();
		if (!fontManager_->Initialize())
		{
			return;
		}
		Gateway::BindFontManager(fontManager_.get());

		loaderSystem_ = MakePtr<LoaderSystem>(device);
		resource_ = MakePtr<ResourceCache>(*loaderSystem_, device, graphics_->GetContext()->GetDirectQueue(), graphics_->GetBindlessHeap());
		resource_->Async();

		Scene::Initialize(*world_, *resource_, *executor_);
		Prefab::Initialize(*world_, *resource_);

		InputSystem::Initialize();
		LayerRegistry::Load();

		std::filesystem::path pluginDirectory = FileDirectory::ExecutableDirectory();
		pluginHost_.Initialize(pluginDirectory, nullptr);
		pluginHost_.Load(*world_);

		ScResolution::ResSize outputSize = ToResSize(gameConfig_.resolution_);
		Uint32 outputWidth = static_cast<Uint32>(outputSize.Width);
		Uint32 outputHeight = static_cast<Uint32>(outputSize.Height);

		Float scale = UpscaleRenderScale(gameConfig_.upscaleMode_);
		Uint32 nativeWidth = Max<Uint32>(64, static_cast<Uint32>(outputWidth * scale + 0.5f));
		Uint32 nativeHeight = Max<Uint32>(64, static_cast<Uint32>(outputHeight * scale + 0.5f));

		graphics_->Resize(nativeWidth, nativeHeight, outputWidth, outputHeight, nullptr);

		graphics_->Reflex(gameConfig_.useReflex_, false);
		graphics_->DeepDVC(gameConfig_.useDeepDVC_, 0.5f, 0.25f);
		graphics_->FrameGeneration(gameConfig_.useFrameGeneration_);

		if (gameConfig_.initialScenePath_.view().empty())
		{
			SC_LOG_ERROR("起動シーンが設定されていません。GameConfig の初回シーンを設定してください");
		}
		else if (!Scene::Change(std::filesystem::path(gameConfig_.initialScenePath_.str())))
		{
			SC_LOG_ERROR("起動シーンの読み込みに失敗しました: {}", gameConfig_.initialScenePath_.str());
		}
	}

	void Engine::Shutdown()
	{
		if (graphics_)
		{
			graphics_->WaitForGpuIdle();
		}

		if (window_)
		{
			window_->RestoreAccessibility();
		}

		if (world_)
		{
			pluginHost_.Unload(*world_);
		}

		InputSystem::Finalize();

		if (resource_)
		{
			resource_.reset();
			resource_ = nullptr;
		}

		if (loaderSystem_)
		{
			loaderSystem_.reset();
			loaderSystem_ = nullptr;
		}

		if (fontManager_)
		{
			fontManager_->Finalize();
			fontManager_.reset();
			fontManager_ = nullptr;
		}

		if (joltManager_)
		{
			joltManager_->Finalize();
			joltManager_.reset();
			joltManager_ = nullptr;
		}

		if (world_)
		{
			world_->GetAudio() = nullptr;
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
				if (window_->ConsumeCloseRequested() || world_->ConsumeQuit())
				{
					return;
				}

				window_->GetTimer().Tick();
				gameTimer_.Tick(window_->GetTimer().Delta());

				InputSystem::Update(true);

				Uint32 resizedWidth = 0;
				Uint32 resizedHeight = 0;
				if (window_->ConsumeResized(resizedWidth, resizedHeight))
				{
					graphics_->ResizeSwapChain(resizedWidth, resizedHeight);
				}

				graphics_->Begin();

				if (!graphics_->SplashFinished())
				{
					resource_->StepAsync(*loaderSystem_, graphics_->GetContext()->GetDevice(), graphics_->GetContext()->GetDirectQueue(), graphics_->GetBindlessHeap(), graphics_->GetBC7CompressShader());

					graphics_->Bind();
					graphics_->DrawSplashScreen(resource_->Complete(), resource_->Progress(), gameConfig_.showSplashWarning_, gameConfig_.showSplashFiction_);
					graphics_->End();
					graphics_->GetSwapChain()->Present(graphics_->GetContext()->GetDevice());
					continue;
				}

				if (!gameTimer_.Playing())
				{
					gameTimer_.Play();
				}

				AudioSystem::ResolveSound(*loaderSystem_, *resource_, *world_);

				/// [EN] Coroutines resume before physics and the script ticks, with this frame's input already read; they do not advance while paused, so frame waits do not count down.
				/// [JP] コルーチンは物理とスクリプトの Tick より前に、今フレームの入力を読んだ後で再開する。ポーズ中は進めないので、フレーム待ちも減らない。
				if (!gameTimer_.Paused())
				{
					CoroutineSystem::Update(gameTimer_.ScaledDeltaTime());
				}

				PhysicsSystem::ResolveMeshCollider(*loaderSystem_, *resource_, *world_);
				PhysicsSystem::ResolveSoftbody(*loaderSystem_, *resource_, *world_);
				PhysicsSystem::ApplyActive(*world_);

				joltManager_->ActiveWorld(world_.get());

				while (gameTimer_.Step())
				{
					joltManager_->Execute(gameTimer_.FixedDeltaTime());
					system_->Step(*world_, gameTimer_.FixedDeltaTime());
				}

				system_->Run(*world_, *resource_, *executor_, gameTimer_.ScaledDeltaTime(), gameTimer_.Playing());

				Scene::Update(gameTimer_.ScaledDeltaTime());

				if (const Scene* switchedScene = Scene::ConsumeSwitchedScene())
				{
					raytracing_ = DeserializeRaytracingContext(switchedScene->Visual().raytracing_);
				}

				AudioSystem::Update(*world_, gameTimer_.ScaledDeltaTime());

				criManager_->Execute();

				if (raytracing_.daySystemEnabled_)
				{
					CelestialSystem::Advance(gameTimer_.ScaledDeltaTime(), raytracing_.daySystem_);

					if (raytracing_.sunLightEnabled_)
					{
						CelestialResult celestial = CelestialSystem::Compute(raytracing_.daySystem_, raytracing_.sunLight_, raytracing_.moonLight_);
						for (Int index = 0; index < 3; index++)
						{
							raytracing_.volumetricCloudScapes_.skyZenithColor_[index] = celestial.skyZenithColor_[index];
							raytracing_.volumetricCloudScapes_.skyHorizonColor_[index] = celestial.skyHorizonColor_[index];
						}
					}
				}
				weatherSystem_.Execute(*world_, gameTimer_.ScaledDeltaTime(), raytracing_.daySystem_.monthOfYear_, raytracing_.volumetricCloudScapes_);

				graphics_->Raytracing(raytracing_);
				graphics_->Upscale(gameConfig_.useDlss_, gameConfig_.upscaleMode_);
				graphics_->VerticalSync(gameConfig_.vsync_);

				graphics_->GameRender(gameTimer_, *loaderSystem_, *resource_, *world_);

				graphics_->DrawLetterScreen();

				graphics_->End();
				graphics_->GetSwapChain()->Present(graphics_->GetContext()->GetDevice());
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
}
