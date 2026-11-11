#pragma once
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct BokehFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool BokehFeature::* enableCondition_ = &BokehFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UpscaleFeature::Color),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(DepthOfFieldFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(BokehFeature& feature, RenderPass& pass);
	};
}
