#pragma once
#include <GraphicsEngine/Light/LightFeature.h>

namespace SeedCore
{
	struct LightFeature::ClusterAssignStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(List),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(ClusterGrid)
		};

		static void Execute(LightFeature& feature, RenderPass& pass);
	};
}
