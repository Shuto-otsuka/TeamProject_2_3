#pragma once
#include <GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Debug/DebugViewFeature.h>

namespace SeedCore
{
	struct RaytracingRefractionFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingRefractionFeature::* enableCondition_ = &RaytracingRefractionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Refraction);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Read(SkyFeature::Cubemap),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Radiance),
			RenderAccess::Append(DebugViewFeature::Rays)
		};

		static void Execute(RaytracingRefractionFeature& feature, RenderPass& pass);
	};
}
