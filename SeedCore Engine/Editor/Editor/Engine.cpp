#include <Editor/Editor/Engine.h>
#include <Editor/Editor/Build/AtomCraft.h>
#include <FoundationEngine/Utility/Bootstrap.h>
#include <FoundationEngine/Time/GameTimer.h>

namespace SeedCore
{
	Bool Engine::Boot(const Bootstrap& bootstrap)
	{
		editorConfig_.Initialize();
		gameConfig_.Initialize();

		if (!core_.BootWindow(bootstrap, editorConfig_))
		{
			return false;
		}

		imgui_ = MakePtr<ImGuiRenderer>();

		JobTaskflow flow;
		core_.BootPhysics(flow);
		core_.BootSplash(flow, editorConfig_);
		core_.BootGraphics(flow, gameConfig_);
		core_.BootAudio(flow, editorConfig_);
		core_.BootFont(flow);
		core_.BootImGui(flow, [this]()
			{
				imgui_->Initialize(core_.GetWindow(), core_.GetGraphics());
				imgui_->FontScale(editorConfig_.fontScale_);
			});
		core_.BootResource(flow);
		core_.BootService(flow);
		core_.BootPrefab(flow);
		core_.BootCsharp(flow);
		core_.BootPlugin(flow);
		core_.BootHotReload(flow, [this]()
			{
				hotReload_.Initialize(core_.GetPlugin(), core_.GetCsharp());
			});
		core_.BootScene(flow, editorConfig_);

		core_.BootEditor(flow, [this]()
			{
				editorContext_.world_.world_ = &core_.GetWorld();
				editorContext_.world_.resource_ = &core_.GetResource();
				editorContext_.world_.sceneSystem_ = &core_.GetScene();
				editorContext_.world_.timer_ = core_.GetWorld().GetService<GameTimer>();
				editorContext_.world_.system_ = &core_.GetSystem();
				editorContext_.view_.editor_.camera_ = &editorCamera_;
				editorContext_.view_.editor_.cameraController_ = &editorCameraController_;
				editorContext_.view_.game_.cameraSystem_ = &core_.GetCamera();
				editorContext_.view_.canvas_.camera_ = &canvasCamera_;
				editorContext_.graphics_.graphics_ = &core_.GetGraphics();
				editorContext_.graphics_.imgui_ = imgui_.get();

				editorCamera_.Eye(editorConfig_.cameraEye_);
				editorCamera_.Focus(editorConfig_.cameraFocus_);
				editorCamera_.Up(editorConfig_.cameraUp_);
				editorCamera_.Fov(editorConfig_.cameraFov_);

				editorCameraController_.MoveSpeed(editorConfig_.cameraMoveSpeed_);
				editorCameraController_.RotateSpeed(editorConfig_.cameraRotateSpeed_);
				editorCameraController_.ScrollSpeed(editorConfig_.cameraScrollSpeed_);
				editorCameraController_.PanSpeed(editorConfig_.cameraPanSpeed_);
				editorCameraController_.ShiftSpeedMultiplier(editorConfig_.cameraShiftSpeedMultiplier_);

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

				editorContext_.config_.game_ = &gameConfig_;
				editorContext_.config_.editor_ = &editorConfig_;
				editorContext_.config_.resizeRequested_ = true;

				if (!core_.GetScene().CurrentName().view().empty())
				{
					editorContext_.scene_.path_ = FilePath(std::filesystem::path(editorConfig_.lastScenePath_.str()), core_.GetResource().ProjectRootPath());
				}
				editor_ = MakePtr<Editor>(editorContext_);
			});

		core_.Execute(flow);

		return true;
	}

	void Engine::Run()
	{
		while (core_.Pump())
		{
			if (core_.GetWindow().ConsumeClosing())
			{
				editorContext_.application_.exitRequested_ = true;
			}

			core_.Tick();
			core_.Input();

			hotReload_.Tick(core_.GetWorld());
			core_.GetPlugin().Tick(core_.GetWorld());

			if (core_.Splash())
			{
				continue;
			}

			editor_->Tick(core_.GetWindow().GetTimer().Delta());
			core_.Begin();
			core_.Simulate(core_.GetWindow().GetTimer().Delta());
			editor_->Draw();

			core_.End();
			core_.Present();
		}
	}

	void Engine::Shutdown()
	{
		core_.GetGraphics().Wait();

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

		editor_ = nullptr;

		if (imgui_)
		{
			imgui_->Finalize();
			imgui_ = nullptr;
		}

		core_.Finalize();
	}
}
