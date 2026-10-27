#include <Runtime/Application/Framework.h>
#include <Runtime/Application/Engine.h>
#include <FoundationEngine/Utility/Bootstrap.h>

namespace SeedCore
{
	Framework::Framework(Engine& engine, const Bootstrap& boot)
	{
		engine_ = &engine;
		engine.Boot(boot);
	}

	Framework::~Framework()
	{
		engine_->Shutdown();
		engine_ = nullptr;
	}

	Int Framework::Run()
	{
		engine_->Run();
		return 0;
	}
}
