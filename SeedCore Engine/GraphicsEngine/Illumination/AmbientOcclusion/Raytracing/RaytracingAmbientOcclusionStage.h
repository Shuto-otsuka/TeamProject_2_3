#pragma once
#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Debug/DebugViewFeature.h>

namespace SeedCore
{
	struct RaytracingAmbientOcclusionFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingAmbientOcclusionFeature::* enableCondition_ = &RaytracingAmbientOcclusionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::AmbientOcclusion);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Raw),
			RenderAccess::Append(DebugViewFeature::RayCount)
		};

		static void Execute(RaytracingAmbientOcclusionFeature& feature, RenderPass& pass);
	};

	struct RaytracingAmbientOcclusionFeature::FilterStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingAmbientOcclusionFeature::* enableCondition_ = &RaytracingAmbientOcclusionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::AmbientOcclusion);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Raw),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Accumulated),
			RenderAccess::Write(Accumulated),
			RenderAccess::Write(Openness),
			RenderAccess::History(HistoryLength),
			RenderAccess::Write(HistoryLength),
			RenderAccess::Write(Validity),
			RenderAccess::Write(Variance)
		};

		static void Execute(RaytracingAmbientOcclusionFeature& feature, RenderPass& pass);
	};
}
