#pragma once
#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobScheduler.h>
#include <FoundationEngine/Application/Window.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/Scene/SceneSystem.h>
#include <FoundationEngine/Resource/Prefab/PrefabSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/System/SystemScheduler.h>
#include <FoundationEngine/Bridge/CsharpHost.h>
#include <FoundationEngine/Plugin/PluginHost.h>

#include <AudioEngine/CRI/CriManager.h>
#include <AudioEngine/Resource/AudioModule.h>

#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Font/FontManager.h>
#include <GraphicsEngine/System/CameraSystem.h>
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	struct Bootstrap;
	class DlssManager;
	class JoltManager;
	struct GameConfig;
	struct EditorConfig;

	class GameTimer;

	class SEEDCORE_API EngineCore :public NonTransferable
	{
	public:
		EngineCore();

		~EngineCore();

		Bool BootWindow(const Bootstrap& bootstrap, const GameConfig& gameConfig);

		Bool BootWindow(const Bootstrap& bootstrap, const EditorConfig& editorConfig);

	public:
		JobTask BootPhysics(JobTaskflow& flow);

		JobTask BootDlss(JobTaskflow& flow);

		JobTask BootAudio(JobTaskflow& flow, const GameConfig& gameConfig);

		JobTask BootAudio(JobTaskflow& flow, const EditorConfig& editorConfig);

		JobTask BootGraphics(JobTaskflow& flow, const GameConfig& gameConfig);

		JobTask BootFont(JobTaskflow& flow);

		JobTask BootResource(JobTaskflow& flow);

		JobTask BootService(JobTaskflow& flow);

		JobTask BootPrefab(JobTaskflow& flow);

		JobTask BootScene(JobTaskflow& flow, const GameConfig& gameConfig);

		JobTask BootScene(JobTaskflow& flow, const EditorConfig& editorConfig);

		JobTask BootCsharp(JobTaskflow& flow);

		JobTask BootPlugin(JobTaskflow& flow);

		JobTask BootSplash(JobTaskflow& flow, const GameConfig& gameConfig);

		JobTask BootSplash(JobTaskflow& flow, const EditorConfig& editorConfig);

		template<typename F>
		JobTask BootHotReload(JobTaskflow& flow, F&& function);

		template<typename F>
		JobTask BootImGui(JobTaskflow& flow, F&& function);

		template<typename F>
		JobTask BootEditor(JobTaskflow& flow, F&& function);

		Bool Execute(JobTaskflow& flow);

	public:
		Bool Pump();

		Bool Quit();

		Bool Splash();

		void Tick();

		void Input();

		void Letterbox(const GameConfig& gameConfig);

		void Begin();

		void End();

		void Play();

		void Simulate(Float deltaTime);

		void Render(const GameConfig& gameConfig);

		void Present();

		void Finalize();

	public:
		NODISCARD Window& GetWindow();

		NODISCARD World& GetWorld();

		NODISCARD SystemScheduler& GetSystem();

		NODISCARD ResourceCache& GetResource();

		NODISCARD Graphics& GetGraphics();

		NODISCARD SceneSystem& GetScene();

		NODISCARD CsharpHost& GetCsharp();

		NODISCARD PluginHost& GetPlugin();

		NODISCARD CameraSystem& GetCamera();

		/**
		* [EN]
		* The CPU section measurements, collected once per frame in Present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CPU の区間計測。Present でフレームに1回集計される。
		*/
		NODISCARD BenchmarkSystem& GetBenchmark();

	private:
		/// [EN] CPU section measurements. Declared first so it is built before executor_, whose worker threads attach to it as they start.
		/// [JP] CPU の区間計測。executor_ のワーカースレッドが起動時に Attach するので、それより先に構築されるよう最初に宣言する。
		BenchmarkSystem benchmark_;

		ResourcePtr<World> world_;

		ResourcePtr<JoltManager> joltManager_;

		ResourcePtr<DlssManager> dlssManager_;

		Window window_;

		JobExecutor executor_;

		SystemScheduler system_;

		ResourceCache resource_;

		CriManager criManager_;

		Graphics graphics_;

		FontManager fontManager_;

		AudioModule audioModule_;

		PrefabSystem prefabSystem_;

		SceneSystem sceneSystem_;

		CsharpHost csharpHost_;

		PluginHost pluginHost_;

		CameraSystem cameraSystem_;

		Vector4 outputRect_ = { 0.0f, 0.0f, 0.0f, 0.0f };

		SplashSystem splashSystem_;

	private:
		std::atomic<Bool> failed_ = false;

		JobTask physicsTask_;

		JobTask dlssTask_;

		JobTask audioTask_;

		JobTask graphicsTask_;

		JobTask fontTask_;

		JobTask resourceTask_;

		JobTask serviceTask_;

		JobTask prefabTask_;

		JobTask sceneTask_;

		JobTask csharpTask_;

		JobTask pluginTask_;

		JobTask splashTask_;

		JobTask hotReloadTask_;

		JobTask imguiTask_;

		JobTask editorTask_;
	};

	template<typename F>
	JobTask EngineCore::BootHotReload(JobTaskflow& flow, F&& function)
	{
		hotReloadTask_ = flow.emplace(std::forward<F>(function));
		pluginTask_.Precede(hotReloadTask_);
		return hotReloadTask_;
	}

	template<typename F>
	JobTask EngineCore::BootImGui(JobTaskflow& flow, F&& function)
	{
		imguiTask_ = flow.emplace(std::forward<F>(function));
		graphicsTask_.Precede(imguiTask_);
		return imguiTask_;
	}

	template<typename F>
	JobTask EngineCore::BootEditor(JobTaskflow& flow, F&& function)
	{
		editorTask_ = flow.emplace(std::forward<F>(function));
		imguiTask_.Precede(editorTask_);
		resourceTask_.Precede(editorTask_);
		serviceTask_.Precede(editorTask_);
		sceneTask_.Precede(editorTask_);
		return editorTask_;
	}
}
