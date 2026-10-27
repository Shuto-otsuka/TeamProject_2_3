#include <SeedCore/Engine/EngineCore.h>
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
#include <AudioEngine/Audio/Audio.h>
#include <AudioEngine/Audio/AudioSystem.h>
#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>

namespace SeedCore
{
	EngineCore::EngineCore() :executor_(std::thread::hardware_concurrency(), MakeWorkerInterface<JobScheduler>()), world_(MakePtr<World>())
	{
		/// No Code
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
				if (!joltManager_.Initialize(executor_))
				{
					failed_ = true;
				}
			});
		return physicsTask_;
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
				graphics_.GetGraphicsModule().Install(resource_);
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
				world_->AddService<Physics>(*queryInstance);
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
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
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

		resource_.StepAsync(graphics_.GetContext().GetDevice(), graphics_.GetContext().GetDirectQueue(), graphics_.GetBC7CompressShader());
		graphics_.DrawSplashScreen(splashSystem_);

		return true;
	}

	void EngineCore::Tick()
	{
		window_.CalculateFrameStats();
		window_.GetTimer().Tick();
		world_->GetService<GameTimer>()->Tick(window_.GetTimer().Delta());
	}

	void EngineCore::Input()
	{
		InputSystem::Update(cameraSystem_.Rect());
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
		Float letterLeft = static_cast<Float>(clientOrigin.x) + (clientWidth - letterWidth) * 0.5f;
		Float letterTop = static_cast<Float>(clientOrigin.y) + (clientHeight - letterHeight) * 0.5f;
		cameraSystem_.Rect(Vector4(letterLeft, letterTop, letterWidth, letterHeight));
	}

	void EngineCore::Begin()
	{
		graphics_.Begin();
	}

	void EngineCore::End()
	{
		graphics_.End();
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
		GameTimer& timer = *world_->GetService<GameTimer>();

		AudioSystem::ResolveSound(audioModule_.Resource(), *world_);

		if (timer.Playing())
		{
			if (!timer.Paused())
			{
				CoroutineSystem::Update(timer.ScaledDeltaTime());
			}

			PhysicsSystem::ResolveMeshCollider(*graphics_.GetGraphicsModule().Resources().meshCollisionResource_, *world_);
			PhysicsSystem::ResolveSoftbody(*graphics_.GetGraphicsModule().Resources().modelResource_, *world_);
			PhysicsSystem::ApplyActive(*world_);
			PhysicsSystem::ApplyCollider(*world_);
			joltManager_.ActiveWorld(world_.get());

			while (timer.Step())
			{
				PhysicsSystem::ApplyRigidbody(*world_, timer.FixedDeltaTime());
				joltManager_.Execute(timer.FixedDeltaTime());
				system_.Step(*world_, timer.FixedDeltaTime());
			}
		}

		system_.Run(*world_, resource_, executor_, deltaTime, timer.ScaledDeltaTime(), timer.Playing());

		if (timer.Playing())
		{
			sceneSystem_.Update(timer.ScaledDeltaTime());
			AudioSystem::Update(*world_, timer.ScaledDeltaTime());
		}

		criManager_.Execute();
	}

	void EngineCore::Render(const GameConfig& gameConfig)
	{
		graphics_.Upscale(gameConfig.useDlss_, gameConfig.upscaleMode_);
		graphics_.VerticalSync(gameConfig.vsync_);
		graphics_.GameRender(*world_->GetService<GameTimer>(), cameraSystem_, *world_);
	}

	void EngineCore::Present()
	{
		graphics_.GetSwapChain().Present(graphics_.GetContext().GetDevice());
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

		joltManager_.Finalize();
		criManager_.Finalize();
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
}
