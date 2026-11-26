#pragma once
#include <FoundationEngine/World/Command/History.h>
#include <FoundationEngine/File/FilePath.h>

namespace SeedCore
{
	struct SceneContext
	{
		FilePath path_;

		Uint32 request_ = 0;

		History history_;
	};
}
