#pragma once
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct HUDFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Canvas;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Target(Display)
		};

		static void Execute(HUDFeature& feature, RenderPass& pass);
	};

	struct HUDFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::All;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Display),
			RenderAccess::Read(Overlay),
			RenderAccess::Constant(Constants),
			RenderAccess::Target(Output)
		};

		static void Execute(HUDFeature& feature, RenderPass& pass);
	};
}
