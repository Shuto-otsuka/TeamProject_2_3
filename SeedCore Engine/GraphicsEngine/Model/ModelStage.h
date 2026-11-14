#pragma once
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/BottomLevelFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>

namespace SeedCore
{
	struct ModelFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool ModelFeature::* enableCondition_ = &ModelFeature::updating_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UnitUpdates),
			RenderAccess::Read(MeshUpdates),
			RenderAccess::Read(ClusterUpdates),
			RenderAccess::Read(PlacementUpdates),
			RenderAccess::Read(MaterialUpdates),
			RenderAccess::Read(TraceUpdates),
			RenderAccess::Write(Units),
			RenderAccess::Write(Meshes),
			RenderAccess::Write(Clusters),
			RenderAccess::Write(Placements),
			RenderAccess::Write(Materials),
			RenderAccess::Write(Traces)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};

	struct ModelFeature::DeformStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool ModelFeature::* enableCondition_ = &ModelFeature::deforming_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Deforms),
			RenderAccess::Read(Units),
			RenderAccess::Read(Meshes),
			RenderAccess::Read(Bones),
			RenderAccess::Read(Morphs),
			RenderAccess::Read(BottomLevelFeature::Table),
			RenderAccess::Append(BottomLevelFeature::Vertices)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};

	struct ModelFeature::SubmitStage
	{
		struct ExpandStep;

		struct ReadbackStep;

		struct RaytracingStep;

		using Sequence = RenderSequence<ExpandStep, ReadbackStep, RaytracingStep>;
	};

	struct ModelFeature::SubmitStage::ExpandStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Meshes),
			RenderAccess::Read(Clusters),
			RenderAccess::Read(Placements),
			RenderAccess::Read(Materials),
			RenderAccess::Read(Softbodies),
			RenderAccess::Constant(Constants),
			RenderAccess::Append(GeometryFeature::Instances),
			RenderAccess::Append(GeometryFeature::Counts),
			RenderAccess::Write(Streaming)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};

	struct ModelFeature::SubmitStage::ReadbackStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Copy;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Streaming)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};

	struct ModelFeature::SubmitStage::RaytracingStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool ModelFeature::* enableCondition_ = &ModelFeature::submitting_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Traces),
			RenderAccess::Read(Meshes),
			RenderAccess::Read(Materials),
			RenderAccess::Append(TopLevelFeature::Instances),
			RenderAccess::Append(TopLevelFeature::Counts)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};
}
