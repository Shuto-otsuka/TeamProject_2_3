#pragma once
#include <SeedCore/Engine/EngineCore.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>

namespace SeedCore
{
	struct Bootstrap;

	class Engine :public NonTransferable
	{
	public:
		Bool Boot(const Bootstrap& bootstrap);

		void Run();

		void Shutdown();

	private:
		EngineCore core_;

		GameConfig gameConfig_;
	};
}
