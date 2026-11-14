#include <SeedCore/Engine/EngineCore.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Coroutine/CoroutineSystem.h>
#include <FoundationEngine/Application/Application.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/Interop/QueryInstance.h>
#include <FoundationEngine/Interop/RenderInstance.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <PhysicsEngine/JoltPhysics/JoltManager.h>
#include <AudioEngine/Audio/Audio.h>
#include <AudioEngine/Audio/AudioSystem.h>
#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>

namespace SeedCore
{
	EngineCore::EngineCore() :executor_(std::thread::hardware_concurrency(), MakeWorkerInterface<JobScheduler>(benchmark_)), world_(MakePtr<World>()), joltManager_(MakePtr<JoltManager>()), dlssManager_(MakePtr<DlssManager>())
	{
		/// [EN] The engine is constructed on the main thread, which runs the frame loop, so the main thread's sections are recorded from here on.
		/// [JP] エンジンはフレームループを回すメインスレッドで構築されるので、ここから先メインスレッドの区間が記録される。
		benchmark_.Attach(String("Main"));
		resource_.Benchmark(benchmark_);
	}

	EngineCore::~EngineCore() = default;

	Bool EngineCore::BootWindow(const Bootstrap& bootstrap, const GameConfig& gameConfig)
	{
		return window_.Initialize(bootstrap, gameConfig);
	}

	Bool EngineCore::BootWindow(const Bootstrap& bootstrap, const EditorConfig& editorConfig)
	{
		return window_.Initialize(bootstrap, editorConfig);
	}

	JobTask EngineCore::BootPhysics(JobTaskflow& flow)
	{
		physicsTask_ = flow.emplace([this]()
			{
				if (!joltManager_->Initialize(executor_))
				{
					failed_ = true;
				}
			});
		return physicsTask_;
	}

	JobTask EngineCore::BootDlss(JobTaskflow& flow)
	{
		dlssTask_ = flow.emplace([this]()
			{
				if (!dlssManager_->Initialize())
				{
					failed_ = true;
				}
			});
		return dlssTask_;
	}

	JobTask EngineCore::BootAudio(JobTaskflow& flow, const GameConfig& gameConfig)
	{
		audioTask_ = flow.emplace([this, gameConfig]()
			{
				if (!criManager_.Initialize(gameConfig))
				{
					failed_ = true;
				}
			});
		return audioTask_;
	}

	JobTask EngineCore::BootAudio(JobTaskflow& flow, const EditorConfig& editorConfig)
	{
		audioTask_ = flow.emplace([this, editorConfig]()
			{
				if (!criManager_.Initialize(editorConfig))
				{
					failed_ = true;
				}
			});
		return audioTask_;
	}

	JobTask EngineCore::BootGraphics(JobTaskflow& flow, const GameConfig& gameConfig)
	{
		graphicsTask_ = flow.emplace([this, gameConfig]()
			{
				if (!graphics_.Initialize(window_, splashSystem_, gameConfig))
				{
					failed_ = true;
				}
			});
		splashTask_.Precede(graphicsTask_);
		dlssTask_.Precede(graphicsTask_);
		return graphicsTask_;
	}

	JobTask EngineCore::BootFont(JobTaskflow& flow)
	{
		fontTask_ = flow.emplace([this]()
			{
				if (!fontManager_.Initialize())
				{
					failed_ = true;
				}
			});
		return fontTask_;
	}

	JobTask EngineCore::BootResource(JobTaskflow& flow)
	{
		resourceTask_ = flow.emplace([this]()
			{
				graphics_.Module().Install(resource_);
				audioModule_.Install(resource_);
				resource_.Async();
			});
		graphicsTask_.Precede(resourceTask_);
		return resourceTask_;
	}

	JobTask EngineCore::BootService(JobTaskflow& flow)
	{
		serviceTask_ = flow.emplace([this]()
			{
				world_->AddService<GameTimer>();
				QueryInstance* queryInstance = world_->AddService<QueryInstance>();
				RenderInstance* renderInstance = world_->AddService<RenderInstance>();
				world_->AddService<Physics>(*world_, *queryInstance);
				world_->AddService<DebugDraw>(*renderInstance);
				world_->AddService<Audio>();
				world_->AddService<Application>();

				LayerRegistry::Load();
			});
		physicsTask_.Precede(serviceTask_);
		audioTask_.Precede(serviceTask_);
		return serviceTask_;
	}

	JobTask EngineCore::BootPrefab(JobTaskflow& flow)
	{
		prefabTask_ = flow.emplace([this]()
			{
				if (!prefabSystem_.Initialize(*world_, resource_))
				{
					failed_ = true;
				}
			});
		resourceTask_.Precede(prefabTask_);
		return prefabTask_;
	}

	JobTask EngineCore::BootScene(JobTaskflow& flow, const GameConfig& gameConfig)
	{
		sceneTask_ = flow.emplace([this, gameConfig]
			{
				if (!sceneSystem_.Initialize(*world_, resource_, executor_, gameConfig))
				{
					failed_ = true;
				}
			});
		prefabTask_.Precede(sceneTask_);
		pluginTask_.Precede(sceneTask_);
		graphicsTask_.Precede(sceneTask_);
		return sceneTask_;
	}

	JobTask EngineCore::BootScene(JobTaskflow& flow, const EditorConfig& editorConfig)
	{
		sceneTask_ = flow.emplace([this, editorConfig]
			{
				if (!sceneSystem_.Initialize(*world_, resource_, executor_, editorConfig))
				{
					failed_ = true;
				}
			});
		prefabTask_.Precede(sceneTask_);
		pluginTask_.Precede(sceneTask_);
		graphicsTask_.Precede(sceneTask_);
		return sceneTask_;
	}

	JobTask EngineCore::BootCsharp(JobTaskflow& flow)
	{
		csharpTask_ = flow.emplace([this]()
			{
				if (!csharpHost_.Initialize(*world_))
				{
					failed_ = true;
				}
			});
		serviceTask_.Precede(csharpTask_);
		return csharpTask_;
	}

	JobTask EngineCore::BootPlugin(JobTaskflow& flow)
	{
		pluginTask_ = flow.emplace([this]()
			{
				if (!pluginHost_.Initialize(*world_))
				{
					failed_ = true;
				}
			});
		csharpTask_.Precede(pluginTask_);

		if (!imguiTask_.Empty())
		{
			imguiTask_.Precede(pluginTask_);
		}

		return pluginTask_;
	}

	JobTask EngineCore::BootSplash(JobTaskflow& flow, const GameConfig& gameConfig)
	{
		splashTask_ = flow.emplace([this, gameConfig]()
			{
				if (!splashSystem_.Initialize(gameConfig))
				{
					failed_ = true;
				}
			});
		return splashTask_;
	}

	JobTask EngineCore::BootSplash(JobTaskflow& flow, const EditorConfig& editorConfig)
	{
		splashTask_ = flow.emplace([this, editorConfig]()
			{
				if (!splashSystem_.Initialize(editorConfig))
				{
					failed_ = true;
				}
			});
		return splashTask_;
	}

	Bool EngineCore::Execute(JobTaskflow& flow)
	{
		JobFuture<void> future = executor_.Run(flow);
		while (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			MSG msg{};
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return !failed_;
	}

	Bool EngineCore::Pump()
	{
		/// [EN] Window messages are measured too, since a slow message handler stalls the whole frame.
		/// [JP] ウィンドウメッセージも測る。遅いメッセージ処理はフレーム全体を止めるため。
		Benchmark::Begin("Pump");
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				Benchmark::End("Pump");
				return false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Benchmark::End("Pump");
		return true;
	}

	Bool EngineCore::Quit()
	{
		return window_.ConsumeClosing() || world_->GetService<Application>()->ConsumeQuit();
	}

	Bool EngineCore::Splash()
	{
		splashSystem_.Update(resource_);
		if (splashSystem_.Complete())
		{
			return false;
		}

		Benchmark::Begin("Splash");
		resource_.StepAsync();
		graphics_.DrawSplashScreen(splashSystem_);
		Benchmark::End("Splash");

		/// [EN] The splash screen presents on its own and skips Present, so the frame is collected here instead.
		/// [JP] スプラッシュ画面は自分で表示して Present を通らないので、ここでフレームを締める。
		benchmark_.Collect();

		return true;
	}

	void EngineCore::Tick()
	{
		Benchmark::Begin("Timer");
		window_.CalculateFrameStats();
		window_.GetTimer().Tick();
		world_->GetService<GameTimer>()->Tick(window_.GetTimer().Delta());
		Benchmark::End("Timer");
	}

	void EngineCore::Input()
	{
		Benchmark::Begin("Input");
		InputSystem::Update(cameraSystem_.Rect());
		Benchmark::End("Input");
	}

	void EngineCore::Letterbox(const GameConfig& gameConfig)
	{
		Uint32 resizeWidth = 0;
		Uint32 resizeHeight = 0;
		if (window_.ConsumeResized(resizeWidth, resizeHeight))
		{
			graphics_.ResizeSwapChain(resizeWidth, resizeHeight);
		}

		RECT clientRect{};
		GetClientRect(window_.Handle(), &clientRect);
		POINT clientOrigin{};
		ClientToScreen(window_.Handle(), &clientOrigin);
		Float clientWidth = static_cast<Float>(clientRect.right - clientRect.left);
		Float clientHeight = static_cast<Float>(clientRect.bottom - clientRect.top);
		ScResolution::ResSize outputSize = ToResSize(gameConfig.resolution_);
		Float outputWidth = static_cast<Float>(outputSize.Width);
		Float outputHeight = static_cast<Float>(outputSize.Height);
		Float letterScale = Min(clientWidth / outputWidth, clientHeight / outputHeight);
		Float letterWidth = outputWidth * letterScale;
		Float letterHeight = outputHeight * letterScale;
		Float letterLeft = (clientWidth - letterWidth) * 0.5f;
		Float letterTop = (clientHeight - letterHeight) * 0.5f;

		/// [EN] The game view is drawn at the rectangle in client coordinates, and input reads it in screen coordinates.
		/// [JP] ゲームビューはクライアント座標の矩形に描き、入力は同じ矩形をスクリーン座標で読む。
		outputRect_ = Vector4(letterLeft, letterTop, letterWidth, letterHeight);
		cameraSystem_.Rect(Vector4(static_cast<Float>(clientOrigin.x) + letterLeft, static_cast<Float>(clientOrigin.y) + letterTop, letterWidth, letterHeight));
	}

	void EngineCore::Begin()
	{
		/// [EN] Includes the wait for the GPU to free this frame's slot, so a GPU-bound frame shows up here.
		/// [JP] GPU がこのフレームの枠を空けるのを待つ時間も含むので、GPU 律速のフレームはここに出る。
		Benchmark::Begin("BeginFrame");
		graphics_.Begin();
		Benchmark::End("BeginFrame");
	}

	void EngineCore::End()
	{
		Benchmark::Begin("EndFrame");
		graphics_.End();
		Benchmark::End("EndFrame");
	}

	void EngineCore::Play()
	{
		GameTimer& timer = *world_->GetService<GameTimer>();
		if (!timer.Playing())
		{
			timer.Play();
		}
	}

	void EngineCore::Simulate(Float deltaTime)
	{
		Benchmark::Begin("Simulate");

		GameTimer& timer = *world_->GetService<GameTimer>();

		Benchmark::Begin("ResolveSound");
		AudioSystem::ResolveSound(audioModule_.Resource(), *world_);
		Benchmark::End("ResolveSound");

		if (timer.Playing())
		{
			if (!timer.Paused())
			{
				Benchmark::Begin("Coroutine");
				CoroutineSystem::Update(timer.ScaledDeltaTime());
				Benchmark::End("Coroutine");
			}

			/// [EN] Physics covers both syncing the world into Jolt and every fixed step, since together they are the cost of simulating the bodies.
			/// [JP] Physics にはワールドから Jolt への反映と全固定ステップの両方を含める。合わせて剛体を動かすコストになるため。
			Benchmark::Begin("Physics");

			Benchmark::Begin("ResolveMeshCollider");
			PhysicsSystem::ResolveMeshCollider(*graphics_.Module().Resources().meshCollisionResource_, *world_);
			Benchmark::End("ResolveMeshCollider");

			Benchmark::Begin("ResolveSoftbody");
			PhysicsSystem::ResolveSoftbody(*graphics_.Module().Resources().modelResource_, *world_);
			Benchmark::End("ResolveSoftbody");

			Benchmark::Begin("ApplyActive");
			PhysicsSystem::ApplyActive(*world_);
			Benchmark::End("ApplyActive");

			Benchmark::Begin("ApplyCollider");
			PhysicsSystem::ApplyCollider(*world_);
			Benchmark::End("ApplyCollider");

			/// [EN] Each fixed step is one call of these sections, so their call count is the number of fixed steps taken this frame.
			/// [JP] 固定ステップ1回がこれらの区間の1呼び出しになるので、回数がこのフレームに進めた固定ステップ数になる。
			while (timer.Step())
			{
				Benchmark::Begin("ApplyRigidbody");
				PhysicsSystem::ApplyRigidbody(*world_, timer.FixedDeltaTime());
				Benchmark::End("ApplyRigidbody");

				Benchmark::Begin("Jolt");
				joltManager_->Execute(timer.FixedDeltaTime(), *world_);
				Benchmark::End("Jolt");

				system_.Step(*world_, timer.FixedDeltaTime());
			}
			Benchmark::End("Physics");
		}

		Benchmark::Begin("Systems");
		system_.Run(*world_, resource_, executor_, deltaTime, timer.ScaledDeltaTime(), timer.Playing());
		Benchmark::End("Systems");

		if (timer.Playing())
		{
			Benchmark::Begin("Scene");
			sceneSystem_.Update(timer.ScaledDeltaTime());
			Benchmark::End("Scene");

			Benchmark::Begin("Audio");
			AudioSystem::Update(*world_, timer.ScaledDeltaTime());
			Benchmark::End("Audio");
		}

		Benchmark::Begin("CriAtom");
		criManager_.Execute();
		Benchmark::End("CriAtom");

		Benchmark::End("Simulate");
	}

	void EngineCore::Render(const GameConfig& gameConfig)
	{
		Benchmark::Begin("Render");
		ScResolution::ResSize outputSize = ToResSize(gameConfig.resolution_);
		GraphicsSettings settings;
		settings.outputWidth_ = static_cast<Uint32>(outputSize.Width);
		settings.outputHeight_ = static_cast<Uint32>(outputSize.Height);
		settings.upscale_.method_ = gameConfig.upscale_.method_;
		settings.upscale_.mode_ = gameConfig.upscale_.mode_;
		settings.verticalSync_ = gameConfig.vsync_;
		settings.frameGeneration_ = gameConfig.useFrameGeneration_;
		graphics_.Configure(settings);
		GameTimer& timer = *world_->GetService<GameTimer>();
		graphics_.Update(timer.ScaledDeltaTime(), *world_);
		graphics_.GameRender(timer, cameraSystem_, *world_, RenderViewOutput::Screen, outputRect_);
		Benchmark::End("Render");
	}

	void EngineCore::Present()
	{
		/// [EN] Graphics::End has already presented the frame. Present is the last step of the frame in both the editor and the runtime, so every section of the frame has ended by here.
		/// [JP] フレームの表示は Graphics::End で済んでいる。Present はエディタでもランタイムでもフレームの最後の手順なので、ここではフレームの区間が全て閉じている。
		benchmark_.Collect();
	}

	void EngineCore::Finalize()
	{
		graphics_.Wait();

		pluginHost_.Unload(*world_);
		csharpHost_.Unload(*world_);

		InputSystem::Finalize();

		resource_.Finalize();
		fontManager_.Finalize();

		if (world_)
		{
			world_.reset();
			world_ = nullptr;
		}

		joltManager_->Finalize();
		criManager_.Finalize();
		dlssManager_->Finalize();
		graphics_.Finalize();
	}

	Window& EngineCore::GetWindow()
	{
		return window_;
	}

	World& EngineCore::GetWorld()
	{
		return *world_;
	}

	SystemScheduler& EngineCore::GetSystem()
	{
		return system_;
	}

	ResourceCache& EngineCore::GetResource()
	{
		return resource_;
	}

	Graphics& EngineCore::GetGraphics()
	{
		return graphics_;
	}

	SceneSystem& EngineCore::GetScene()
	{
		return sceneSystem_;
	}

	CsharpHost& EngineCore::GetCsharp()
	{
		return csharpHost_;
	}

	PluginHost& EngineCore::GetPlugin()
	{
		return pluginHost_;
	}

	CameraSystem& EngineCore::GetCamera()
	{
		return cameraSystem_;
	}

	/**
	* [EN]
	* The CPU section measurements, collected once per frame in Present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CPU の区間計測。Present でフレームに1回集計される。
	*/
	BenchmarkSystem& EngineCore::GetBenchmark()
	{
		return benchmark_;
	}
}
