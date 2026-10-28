#include <Runtime/Application/Engine.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads the game settings, opens the window, and runs every boot
	* step as one task graph. Returns false when any step fails.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームの設定を読み、ウィンドウを開き、全ての起動手順を1つの
	* タスクグラフとして走らせる。どれかの手順が失敗すれば false を返す。
	*/
	Bool Engine::Boot(const Bootstrap& bootstrap)
	{
		gameConfig_.Initialize();

		/// [EN] The window comes first and alone, since every later step draws into it or reads its handle.
		/// [JP] 後の手順はどれもウィンドウへ描くかそのハンドルを読むので、ウィンドウだけを最初に作る。
		if (!core_.BootWindow(bootstrap, gameConfig_))
		{
			return false;
		}

		/// [EN] Each Boot call adds its step to the graph with the steps it waits on; nothing runs until Execute.
		/// [JP] 各 Boot 呼び出しは、待つ手順と一緒に自分の手順をグラフへ足すだけで、Execute まで何も走らない。
		JobTaskflow flow;
		core_.BootPhysics(flow);
		core_.BootSplash(flow, gameConfig_);
		core_.BootGraphics(flow, gameConfig_);
		core_.BootAudio(flow, gameConfig_);
		core_.BootFont(flow);
		core_.BootResource(flow);
		core_.BootService(flow);
		core_.BootPrefab(flow);
		core_.BootCsharp(flow);
		core_.BootPlugin(flow);
		core_.BootScene(flow, gameConfig_);
		return core_.Execute(flow);
	}

	/**
	* [EN]
	* Runs the game loop until the window closes or the game asks to
	* quit.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ウィンドウが閉じるか、ゲームが終了を求めるまでゲームのループを回す。
	*/
	void Engine::Run()
	{
		while (core_.Pump() && !core_.Quit())
		{
			core_.Tick();
			core_.Input();

			/// [EN] Fits the game's output resolution into the window with black bars, keeping its aspect ratio.
			/// [JP] ゲームの出力解像度を、縦横比を保ったまま黒帯付きでウィンドウへ収める。
			core_.Letterbox(gameConfig_);

			/// [EN] While the splash screen is up, the frame is spent loading assets instead.
			/// [JP] スプラッシュ画面の間は、フレームをアセットの読み込みに使う。
			if (core_.Splash())
			{
				continue;
			}
			core_.Begin();

			/// [EN] The runtime is always playing; the game timer starts on the first frame after the splash.
			/// [JP] ランタイムは常にプレイ中。ゲームのタイマーはスプラッシュ後の最初のフレームで動き出す。
			core_.Play();

			/// [EN] The runtime has no editor-side time, so 0 is passed; the game advances on the game timer.
			/// [JP] ランタイムにはエディタ側の時間が無いので 0 を渡す。ゲームはゲームのタイマーで進む。
			core_.Simulate(0.0f);
			core_.Render(gameConfig_);
			core_.End();
			core_.Present();
		}
	}

	/**
	* [EN]
	* Releases every module of the core.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コアの全モジュールを解放する。
	*/
	void Engine::Shutdown()
	{
		core_.Finalize();
	}
}
