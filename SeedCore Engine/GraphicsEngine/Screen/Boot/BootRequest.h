#pragma once
#include <FoundationEngine/Resource/Config/BootConfig.h>

namespace SeedCore
{
	class BootTexture;

	struct BootRequest
	{
		Float alpha_ = 1.0f;

		Float progress_ = 0.0f;

		Float time_ = 0.0f;

		const BootConfig* config_ = nullptr;

		const BootTexture* texture_ = nullptr;
	};
}
