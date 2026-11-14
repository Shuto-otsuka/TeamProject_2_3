#pragma once
#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Debug/DebugViewFeature.h>

namespace SeedCore
{
	struct RaytracingShadowFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingShadowFeature::* enableCondition_ = &RaytracingShadowFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Shadow);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(LightFeature::List),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Read(LightFeature::Clusters),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Raw),
			RenderAccess::Append(DebugViewFeature::Rays)
		};

		static void Execute(RaytracingShadowFeature& feature, RenderPass& pass);
	};

	struct RaytracingShadowFeature::FilterStage
	{
		struct TemporalStep;

		struct MomentsStep;

		struct ATrousStep;

		using Sequence = RenderSequence<TemporalStep, MomentsStep, ATrousStep>;
	};

	struct RaytracingShadowFeature::FilterStage::TemporalStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingShadowFeature::* enableCondition_ = &RaytracingShadowFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Shadow);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Raw),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Accumulated),
			RenderAccess::History(Moments),
			RenderAccess::Write(Moments),
			RenderAccess::History(Age),
			RenderAccess::Write(Age),
			RenderAccess::History(DepthNormal),
			RenderAccess::Write(DepthNormal),
			RenderAccess::Write(Ping),
			RenderAccess::Write(Validity)
		};

		static void Execute(RaytracingShadowFeature& feature, RenderPass& pass);
	};

	struct RaytracingShadowFeature::FilterStage::MomentsStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingShadowFeature::* enableCondition_ = &RaytracingShadowFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Shadow);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Ping),
			RenderAccess::Read(Moments),
			RenderAccess::Read(Age),
			RenderAccess::Read(DepthNormal),
			RenderAccess::Write(Pong)
		};

		static void Execute(RaytracingShadowFeature& feature, RenderPass& pass);
	};

	struct RaytracingShadowFeature::FilterStage::ATrousStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingShadowFeature::* enableCondition_ = &RaytracingShadowFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Shadow);

		SC_CONSTEX Uint32 repeat_ = 3;

		SC_CONSTEX RenderAccess accesses_[][3] =
		{
			{
				RenderAccess::Read(Pong).As("Source"),
				RenderAccess::Read(DepthNormal),
				RenderAccess::Write(Ping).As("Destination")
			},
			{
				RenderAccess::Read(Ping).As("Source"),
				RenderAccess::Read(DepthNormal),
				RenderAccess::Write(Accumulated).As("Destination")
			},
			{
				RenderAccess::Read(Accumulated).As("Source"),
				RenderAccess::Read(DepthNormal),
				RenderAccess::Write(Visibility).As("Destination")
			}
		};

		static void Execute(RaytracingShadowFeature& feature, RenderPass& pass);
	};
}
