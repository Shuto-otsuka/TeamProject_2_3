#pragma once
#include <GraphicsEngine/Shape/Primitive/PrimitiveFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>

namespace SeedCore
{
	struct PrimitiveFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool PrimitiveFeature::* enableCondition_ = &PrimitiveFeature::updating_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(SolidUpdates),
			RenderAccess::Read(MaterialUpdates),
			RenderAccess::Write(Solids),
			RenderAccess::Write(Materials)
		};

		static void Execute(PrimitiveFeature& feature, RenderPass& pass);
	};

	struct PrimitiveFeature::SubmitStage
	{
		struct ExpandStep;

		struct RaytracingStep;

		using Sequence = RenderSequence<ExpandStep, RaytracingStep>;
	};

	struct PrimitiveFeature::SubmitStage::ExpandStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool PrimitiveFeature::* enableCondition_ = &PrimitiveFeature::submitting_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Solids),
			RenderAccess::Read(Materials),
			RenderAccess::Constant(Constants),
			RenderAccess::Append(GeometryFeature::Instances),
			RenderAccess::Append(GeometryFeature::Counts)
		};

		static void Execute(PrimitiveFeature& feature, RenderPass& pass);
	};

	struct PrimitiveFeature::SubmitStage::RaytracingStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool PrimitiveFeature::* enableCondition_ = &PrimitiveFeature::submitting_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Solids),
			RenderAccess::Read(Materials),
			RenderAccess::Constant(Constants),
			RenderAccess::Append(TopLevelFeature::Instances),
			RenderAccess::Append(TopLevelFeature::Counts)
		};

		static void Execute(PrimitiveFeature& feature, RenderPass& pass);
	};

	struct PrimitiveFeature::DrawStage
	{
		struct WireframeStep;

		struct CanvasStep;

		using Sequence = RenderSequence<WireframeStep, CanvasStep>;
	};

	struct PrimitiveFeature::DrawStage::WireframeStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Scene;

		SC_CONSTEX Bool(PrimitiveFeature::* enableCondition_)[RenderViewCount] = &PrimitiveFeature::drawing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Wireframes),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Target(HUDFeature::Display)
		};

		static void Execute(PrimitiveFeature& feature, RenderPass& pass);
	};

	struct PrimitiveFeature::DrawStage::CanvasStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Canvas;

		SC_CONSTEX Bool(PrimitiveFeature::* enableCondition_)[RenderViewCount] = &PrimitiveFeature::drawing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Wireframes),
			RenderAccess::Target(HUDFeature::Display)
		};

		static void Execute(PrimitiveFeature& feature, RenderPass& pass);
	};
}
