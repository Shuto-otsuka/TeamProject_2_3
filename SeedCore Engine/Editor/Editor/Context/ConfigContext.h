#pragma once
#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>

namespace SeedCore
{
	struct ConfigContext
	{
		GameConfig* game_ = nullptr;

		EditorConfig* editor_ = nullptr;
	};
}
