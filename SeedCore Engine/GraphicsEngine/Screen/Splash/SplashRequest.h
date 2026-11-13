#pragma once
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	class SplashTexture;

	struct SplashRequest
	{
		SplashPhase phase_ = SplashPhase::Warning;

		Float alpha_ = 1.0f;

		const SplashTexture* texture_ = nullptr;
	};
}
