#pragma once
#include <GraphicsEngine/PostProcess/LensDistortion/LensDistortionFeature.h>

namespace SeedCore
{
	struct LensDistortionFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensDistortionFeature::* enableCondition_ = &LensDistortionFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::LensDistortion);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(LensDistortionFeature& feature, RenderPass& pass);
	};
}
