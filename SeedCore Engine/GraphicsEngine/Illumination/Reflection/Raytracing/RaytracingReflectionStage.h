#pragma once
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Debug/DebugViewFeature.h>

namespace SeedCore
{
	struct RaytracingReflectionFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raytracing;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingReflectionFeature::* enableCondition_ = &RaytracingReflectionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Reflection);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Read(LightFeature::List),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Read(LightFeature::ClusterGrid),
			RenderAccess::Read(SkyFeature::Cubemap),
			RenderAccess::Read(SkyFeature::DiffuseIrradiance),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Reservoir),
			RenderAccess::Write(Reservoir),
			RenderAccess::Write(Reuse),
			RenderAccess::Append(DebugViewFeature::RayCount)
		};

		static void Execute(RaytracingReflectionFeature& feature, RenderPass& pass);
	};

	struct RaytracingReflectionFeature::FilterStage
	{
		struct SpatialStep;

		struct TemporalStep;

		struct MomentsStep;

		struct ATrousStep;

		using Sequence = RenderSequence<SpatialStep, TemporalStep, MomentsStep, ATrousStep>;
	};

	struct RaytracingReflectionFeature::FilterStage::SpatialStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingReflectionFeature::* enableCondition_ = &RaytracingReflectionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Reflection);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(Reservoir),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Raw),
			RenderAccess::Write(Confidence),
			RenderAccess::Write(Reuse)
		};

		static void Execute(RaytracingReflectionFeature& feature, RenderPass& pass);
	};

	struct RaytracingReflectionFeature::FilterStage::TemporalStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingReflectionFeature::* enableCondition_ = &RaytracingReflectionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Reflection);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Raw),
			RenderAccess::Read(Confidence),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Accumulated),
			RenderAccess::History(Moments),
			RenderAccess::Write(Moments),
			RenderAccess::History(HistoryLength),
			RenderAccess::Write(HistoryLength),
			RenderAccess::History(DepthNormal),
			RenderAccess::Write(DepthNormal),
			RenderAccess::Write(Ping),
			RenderAccess::Write(Validity)
		};

		static void Execute(RaytracingReflectionFeature& feature, RenderPass& pass);
	};

	struct RaytracingReflectionFeature::FilterStage::MomentsStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingReflectionFeature::* enableCondition_ = &RaytracingReflectionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Reflection);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Ping),
			RenderAccess::Read(Moments),
			RenderAccess::Read(HistoryLength),
			RenderAccess::Read(DepthNormal),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Write(Pong)
		};

		static void Execute(RaytracingReflectionFeature& feature, RenderPass& pass);
	};

	struct RaytracingReflectionFeature::FilterStage::ATrousStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingReflectionFeature::* enableCondition_ = &RaytracingReflectionFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Reflection);

		SC_CONSTEX Uint32 repeat_ = 3;

		SC_CONSTEX RenderAccess accesses_[][5] =
		{
			{
				RenderAccess::Read(Pong).As("Source"),
				RenderAccess::Read(DepthNormal),
				RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
				RenderAccess::Constant(Constants),
				RenderAccess::Write(Ping).As("Destination")
			},
			{
				RenderAccess::Read(Ping).As("Source"),
				RenderAccess::Read(DepthNormal),
				RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
				RenderAccess::Constant(Constants),
				RenderAccess::Write(Accumulated).As("Destination")
			},
			{
				RenderAccess::Read(Accumulated).As("Source"),
				RenderAccess::Read(DepthNormal),
				RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
				RenderAccess::Constant(Constants),
				RenderAccess::Write(Radiance).As("Destination")
			}
		};

		static void Execute(RaytracingReflectionFeature& feature, RenderPass& pass);
	};
}
