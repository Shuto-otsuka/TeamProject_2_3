#pragma once
#include <GraphicsEngine/Screen/Splash/SplashFeature.h>

namespace SeedCore
{
	struct SplashFeature::DrawStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::All;

		SC_CONSTEX Bool SplashFeature::* enableCondition_ = &SplashFeature::drawing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Target(Output)
		};

		static void Execute(SplashFeature& feature, RenderPass& pass);
	};
}
