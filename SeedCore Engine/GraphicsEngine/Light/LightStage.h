#pragma once
#include <GraphicsEngine/Light/LightFeature.h>

namespace SeedCore
{
	struct LightFeature::BuildStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(List),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Clusters)
		};

		static void Execute(LightFeature& feature, RenderPass& pass);
	};
}
