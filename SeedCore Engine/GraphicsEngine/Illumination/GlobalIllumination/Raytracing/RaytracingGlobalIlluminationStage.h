#pragma once
#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Debug/DebugViewFeature.h>

namespace SeedCore
{
	struct RaytracingGlobalIlluminationFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raytracing;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingGlobalIlluminationFeature::* enableCondition_ = &RaytracingGlobalIlluminationFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::GlobalIllumination);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Read(LightFeature::List),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Read(LightFeature::Clusters),
			RenderAccess::Read(SkyFeature::Cubemap),
			RenderAccess::Read(SkyFeature::Diffuse),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Reservoir),
			RenderAccess::Write(Reservoir),
			RenderAccess::Write(Reuse),
			RenderAccess::Append(DebugViewFeature::Rays)
		};

		static void Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass);
	};

	struct RaytracingGlobalIlluminationFeature::FilterStage
	{
		struct SpatialStep;

		struct TemporalStep;

		struct ATrousStep;

		using Sequence = RenderSequence<SpatialStep, TemporalStep, ATrousStep>;
	};

	struct RaytracingGlobalIlluminationFeature::FilterStage::SpatialStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingGlobalIlluminationFeature::* enableCondition_ = &RaytracingGlobalIlluminationFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::GlobalIllumination);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(Reservoir),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Raw),
			RenderAccess::Write(Confidence),
			RenderAccess::Write(Reuse)
		};

		static void Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass);
	};

	struct RaytracingGlobalIlluminationFeature::FilterStage::TemporalStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingGlobalIlluminationFeature::* enableCondition_ = &RaytracingGlobalIlluminationFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::GlobalIllumination);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Raw),
			RenderAccess::Read(Confidence),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Accumulated),
			RenderAccess::Write(Ping),
			RenderAccess::History(Age),
			RenderAccess::Write(Age),
			RenderAccess::Write(Validity),
			RenderAccess::Write(Variance)
		};

		static void Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass);
	};

	struct RaytracingGlobalIlluminationFeature::FilterStage::ATrousStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingGlobalIlluminationFeature::* enableCondition_ = &RaytracingGlobalIlluminationFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::GlobalIllumination);

		SC_CONSTEX Uint32 repeat_ = 3;

		SC_CONSTEX RenderAccess accesses_[][4] =
		{
			{
				RenderAccess::Read(Ping).As("Source"),
				RenderAccess::Read(GeometryFeature::Depth),
				RenderAccess::Read(MaterialFeature::NormalRoughness),
				RenderAccess::Write(Pong).As("Destination")
			},
			{
				RenderAccess::Read(Pong).As("Source"),
				RenderAccess::Read(GeometryFeature::Depth),
				RenderAccess::Read(MaterialFeature::NormalRoughness),
				RenderAccess::Write(Accumulated).As("Destination")
			},
			{
				RenderAccess::Read(Accumulated).As("Source"),
				RenderAccess::Read(GeometryFeature::Depth),
				RenderAccess::Read(MaterialFeature::NormalRoughness),
				RenderAccess::Write(Radiance).As("Destination")
			}
		};

		static void Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass);
	};
}
