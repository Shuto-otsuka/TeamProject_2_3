#pragma once
#include <GraphicsEngine/PostProcess/Sharpness/SharpnessFeature.h>

namespace SeedCore
{
	struct SharpnessFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool SharpnessFeature::* enableCondition_ = &SharpnessFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ToneMappingFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(SharpnessFeature& feature, RenderPass& pass);
	};
}
