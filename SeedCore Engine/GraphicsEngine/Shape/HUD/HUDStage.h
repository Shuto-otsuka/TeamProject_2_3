#pragma once
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct HUDFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::All;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(DisplayColor),
			RenderAccess::Read(Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Target(ViewOutput)
		};

		static void Execute(HUDFeature& feature, RenderPass& pass);
	};
}
