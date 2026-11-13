#pragma once
#include <GraphicsEngine/Raytracing/BottomLevelFeature.h>

namespace SeedCore
{
	struct BottomLevelFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool BottomLevelFeature::* enableCondition_ = &BottomLevelFeature::updating_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(TableUpdates),
			RenderAccess::Write(Table)
		};

		static void Execute(BottomLevelFeature& feature, RenderPass& pass);
	};

	struct BottomLevelFeature::BuildStage
	{
		struct StaticStep;

		struct DynamicStep;

		using Sequence = RenderSequence<StaticStep, DynamicStep>;
	};

	struct BottomLevelFeature::BuildStage::StaticStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Build;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool BottomLevelFeature::* enableCondition_ = &BottomLevelFeature::buildingStatic_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Table)
		};

		static void Execute(BottomLevelFeature& feature, RenderPass& pass);
	};

	struct BottomLevelFeature::BuildStage::DynamicStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Build;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool BottomLevelFeature::* enableCondition_ = &BottomLevelFeature::buildingDynamic_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Vertices)
		};

		static void Execute(BottomLevelFeature& feature, RenderPass& pass);
	};
}
