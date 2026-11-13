#pragma once
#include <GraphicsEngine/PostProcess/DepthOfField/DepthOfFieldFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct DepthOfFieldFeature::FilterStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool DepthOfFieldFeature::* enableCondition_ = &DepthOfFieldFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::DepthOfField);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UpscaleFeature::Color),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(DepthOfFieldFeature& feature, RenderPass& pass);
	};
}
