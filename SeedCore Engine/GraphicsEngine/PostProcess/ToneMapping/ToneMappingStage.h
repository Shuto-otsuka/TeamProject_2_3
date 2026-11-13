#pragma once
#include <GraphicsEngine/PostProcess/ToneMapping/ToneMappingFeature.h>

namespace SeedCore
{
	struct ToneMappingFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::ToneMapping);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ColorGradingFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(ToneMappingFeature& feature, RenderPass& pass);
	};
}
