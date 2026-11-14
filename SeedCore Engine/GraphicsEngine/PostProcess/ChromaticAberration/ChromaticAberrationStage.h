#pragma once
#include <GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationFeature.h>

namespace SeedCore
{
	struct ChromaticAberrationFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool ChromaticAberrationFeature::* enableCondition_ = &ChromaticAberrationFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::ChromaticAberration);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(LensDistortionFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(ChromaticAberrationFeature& feature, RenderPass& pass);
	};
}
