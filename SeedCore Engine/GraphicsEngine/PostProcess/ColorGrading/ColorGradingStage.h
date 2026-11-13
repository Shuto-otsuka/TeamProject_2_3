#pragma once
#include <GraphicsEngine/PostProcess/ColorGrading/ColorGradingFeature.h>

namespace SeedCore
{
	struct ColorGradingFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool ColorGradingFeature::* enableCondition_ = &ColorGradingFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::ColorGrading);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(PostProcessFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(ColorGradingFeature& feature, RenderPass& pass);
	};
}
