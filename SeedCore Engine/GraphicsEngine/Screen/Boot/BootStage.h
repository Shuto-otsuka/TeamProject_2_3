#pragma once
#include <GraphicsEngine/Screen/Boot/BootFeature.h>

namespace SeedCore
{
	struct BootFeature::DrawStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::All;

		SC_CONSTEX Bool BootFeature::* enableCondition_ = &BootFeature::drawing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Target(Output)
		};

		static void Execute(BootFeature& feature, RenderPass& pass);
	};
}
