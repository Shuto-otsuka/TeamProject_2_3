#pragma once
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStarFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>

namespace SeedCore
{
	struct VolumetricStarFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool VolumetricStarFeature::* enableCondition_ = &VolumetricStarFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::VolumetricStar);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Write(Radiance)
		};

		static void Execute(VolumetricStarFeature& feature, RenderPass& pass);
	};
}
