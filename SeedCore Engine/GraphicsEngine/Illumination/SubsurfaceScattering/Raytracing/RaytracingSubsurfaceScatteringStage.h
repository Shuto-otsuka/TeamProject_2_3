#pragma once
#include <GraphicsEngine/Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Debug/DebugViewFeature.h>

namespace SeedCore
{
	struct RaytracingSubsurfaceScatteringFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingSubsurfaceScatteringFeature::* enableCondition_ = &RaytracingSubsurfaceScatteringFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::SubsurfaceScattering);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Transmittance),
			RenderAccess::Append(DebugViewFeature::RayCount)
		};

		static void Execute(RaytracingSubsurfaceScatteringFeature& feature, RenderPass& pass);
	};
}
