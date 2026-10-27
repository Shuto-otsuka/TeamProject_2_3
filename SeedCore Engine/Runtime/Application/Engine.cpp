#include <Runtime/Application/Engine.h>

namespace SeedCore
{
	Bool Engine::Boot(const Bootstrap& bootstrap)
	{
		gameConfig_.Initialize();

		if (!core_.BootWindow(bootstrap, gameConfig_))
		{
			return false;
		}

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

	void Engine::Run()
	{
		while (core_.Pump() && !core_.Quit())
		{
			core_.Tick();
			core_.Input();
			core_.Letterbox(gameConfig_);
			if (core_.Splash())
			{
				continue;
			}
			core_.Begin();
			core_.Play();
			core_.Simulate(0.0f);
			core_.Render(gameConfig_);
			core_.End();
			core_.Present();
		}
	}

	void Engine::Shutdown()
	{
		core_.Finalize();
	}
}
