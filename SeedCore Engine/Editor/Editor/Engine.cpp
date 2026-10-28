#include <Editor/Editor/Engine.h>
#include <Editor/Editor/Build/AtomCraft.h>
#include <FoundationEngine/Utility/Bootstrap.h>
#include <FoundationEngine/Time/GameTimer.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads the settings, opens the window, and runs every boot step
	* as one task graph. Returns false when the window cannot be made.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 設定を読み、ウィンドウを開き、全ての起動手順を1つのタスクグラフ
	* として走らせる。ウィンドウを作れなければ false を返す。
	*/
	Bool Engine::Boot(const Bootstrap& bootstrap)
	{
		editorConfig_.Initialize();
		gameConfig_.Initialize();

		/// [EN] The window comes first and alone, since every later step draws into it or reads its handle.
		/// [JP] 後の手順はどれもウィンドウへ描くかそのハンドルを読むので、ウィンドウだけを最初に作る。
		if (!core_.BootWindow(bootstrap, editorConfig_))
		{
			return false;
		}

		imgui_ = MakePtr<ImGuiRenderer>();

		/// [EN] Each Boot call adds its step to the graph with the steps it waits on; nothing runs until Execute.
		/// [JP] 各 Boot 呼び出しは、待つ手順と一緒に自分の手順をグラフへ足すだけで、Execute まで何も走らない。
		JobTaskflow flow;
		core_.BootPhysics(flow);
		core_.BootSplash(flow, editorConfig_);
		core_.BootGraphics(flow, gameConfig_);
		core_.BootAudio(flow, editorConfig_);
		core_.BootFont(flow);

		/// [EN] Editor-only step: ImGui needs the window and the graphics device.
		/// [JP] エディタだけの手順。ImGui にはウィンドウとグラフィックスデバイスが要る。
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

		/// [EN] Editor-only step: hot reload watches the native plugin and the C# host once both are loaded.
		/// [JP] エディタだけの手順。ネイティブのプラグインと C# のホストが読み込まれてから、ホットリロードがそれらを見張る。
		core_.BootHotReload(flow, [this]()
			{
				hotReload_.Initialize(core_.GetPlugin(), core_.GetCsharp());
			});
		core_.BootScene(flow, editorConfig_);

		/// [EN] Editor-only last step: wires the editor context to the booted modules and creates the editor UI.
		/// [JP] エディタだけの最後の手順。起動したモジュールにエディタのコンテキストをつなぎ、エディタの UI を作る。
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

				/// [EN] The editor camera resumes where it was when the editor last closed.
				/// [JP] エディタのカメラは、前回エディタを閉じたときの状態から再開する。
				editorCamera_.Eye(editorConfig_.cameraEye_);
				editorCamera_.Focus(editorConfig_.cameraFocus_);
				editorCamera_.Up(editorConfig_.cameraUp_);
				editorCamera_.Fov(editorConfig_.cameraFov_);

				editorCameraController_.MoveSpeed(editorConfig_.cameraMoveSpeed_);
				editorCameraController_.RotateSpeed(editorConfig_.cameraRotateSpeed_);
				editorCameraController_.ScrollSpeed(editorConfig_.cameraScrollSpeed_);
				editorCameraController_.PanSpeed(editorConfig_.cameraPanSpeed_);
				editorCameraController_.ShiftSpeedMultiplier(editorConfig_.cameraShiftSpeedMultiplier_);

				/// [EN] A missing or moved CRI Atom Craft is looked for again; a found path is saved so the search runs only once.
				/// [JP] CRI Atom Craft が未設定か見つからなければ探し直し、見つかったパスは保存して探すのを1度で済ませる。
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

				/// [EN] The first editor frame applies the configured resolution.
				/// [JP] 最初のエディタのフレームで、設定した解像度を反映させる。
				editorContext_.config_.game_ = &gameConfig_;
				editorContext_.config_.editor_ = &editorConfig_;
				editorContext_.config_.resizeRequested_ = true;

				/// [EN] With a scene loaded, the editor reopens the scene file that was open last time.
				/// [JP] Scene が読み込まれていれば、前回開いていた Scene ファイルをエディタで開き直す。
				if (!core_.GetScene().CurrentName().view().empty())
				{
					editorContext_.scene_.path_ = FilePath(std::filesystem::path(editorConfig_.lastScenePath_.str()), core_.GetResource().ProjectRootPath());
				}
				editor_ = MakePtr<Editor>(editorContext_);
			});

		/// [EN] Runs the whole graph, pumping window messages while it works.
		/// [JP] ウィンドウのメッセージを処理しながら、グラフ全体を走らせる。
		core_.Execute(flow);

		return true;
	}

	/**
	* [EN]
	* Runs the editor loop until the window's message queue ends.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ウィンドウのメッセージキューが終わるまでエディタのループを回す。
	*/
	void Engine::Run()
	{
		while (core_.Pump())
		{
			/// [EN] Closing the window does not end the loop directly; it is handed to the editor as an exit request.
			/// [JP] ウィンドウを閉じる操作でループを直接終わらせず、終了要求としてエディタへ渡す。
			if (core_.GetWindow().ConsumeClosing())
			{
				editorContext_.application_.exitRequested_ = true;
			}

			core_.Tick();
			core_.Input();

			/// [EN] Reloaded code is swapped in before the frame, so this frame already runs the new code.
			/// [JP] 読み直したコードはフレームの前に差し替え、このフレームから新しいコードで動かす。
			hotReload_.Tick(core_.GetWorld());
			core_.GetPlugin().Tick(core_.GetWorld());

			/// [EN] While the splash screen is up, the frame is spent loading assets instead.
			/// [JP] スプラッシュ画面の間は、フレームをアセットの読み込みに使う。
			if (core_.Splash())
			{
				continue;
			}

			/// [EN] The editor's own delta time drives editor-side work, which keeps going while the game is stopped.
			/// [JP] エディタ自身の経過時間でエディタ側の処理を進め、ゲームが止まっていても動き続けるようにする。
			editor_->Tick(core_.GetWindow().GetTimer().Delta());
			core_.Begin();
			core_.Simulate(core_.GetWindow().GetTimer().Delta());
			editor_->Draw();

			core_.End();
			core_.Present();
		}
	}

	/**
	* [EN]
	* Saves the editor's camera, font scale and last scene, then
	* releases the editor, ImGui and the core in that order.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタのカメラ、フォント倍率、最後の Scene を保存し、続けて
	* エディタ、ImGui、コアの順に解放する。
	*/
	void Engine::Shutdown()
	{
		/// [EN] The GPU must finish with every resource before any of them is released.
		/// [JP] どのリソースを解放するよりも前に、GPU にそれらを使い終えさせる。
		core_.GetGraphics().Wait();

		/// [EN] ImGui exists only once boot got past the window, so a boot that failed there leaves the saved settings alone.
		/// [JP] ImGui があるのはウィンドウの作成を越えて起動できたときだけ。そこで失敗した起動では、保存済みの設定に触れない。
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

		/// [EN] The editor goes before ImGui and the core, since its panels refer to both.
		/// [JP] エディタのパネルは ImGui とコアの両方を参照しているので、エディタを先に解放する。
		editor_ = nullptr;

		if (imgui_)
		{
			imgui_->Finalize();
			imgui_ = nullptr;
		}

		core_.Finalize();
	}
}
