#pragma once
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Raytracing/BottomLevelFeature.h>

namespace SeedCore
{
	struct TopLevelFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Counts)
		};

		static void Execute(TopLevelFeature& feature, RenderPass& pass);
	};

	struct TopLevelFeature::BuildStage
	{
		struct ResolveStep;

		struct StructureStep;

		using Sequence = RenderSequence<ResolveStep, StructureStep>;
	};

	struct TopLevelFeature::BuildStage::ResolveStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Instances),
			RenderAccess::Read(Counts),
			RenderAccess::Read(BottomLevelFeature::Table),
			RenderAccess::Write(Descs),
			RenderAccess::Write(Hits)
		};

		static void Execute(TopLevelFeature& feature, RenderPass& pass);
	};

	struct TopLevelFeature::BuildStage::StructureStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Build;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Descs),
			RenderAccess::Write(Structure)
		};

		static void Execute(TopLevelFeature& feature, RenderPass& pass);
	};
}
