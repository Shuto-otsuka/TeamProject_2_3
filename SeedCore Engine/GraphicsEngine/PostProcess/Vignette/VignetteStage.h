#pragma once
#include <GraphicsEngine/PostProcess/Vignette/VignetteFeature.h>

namespace SeedCore
{
	struct VignetteFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool VignetteFeature::* enableCondition_ = &VignetteFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::Vignette);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ChromaticAberrationFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(VignetteFeature& feature, RenderPass& pass);
	};
}
