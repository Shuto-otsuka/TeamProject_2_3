#pragma once
#include <GraphicsEngine/PostProcess/Outline/OutlineFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct OutlineFeature::UpdateStage
	{
		struct StyleStep;

		struct MaskStep;

		using Sequence = RenderSequence<StyleStep, MaskStep>;
	};

	struct OutlineFeature::UpdateStage::StyleStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool OutlineFeature::* enableCondition_ = &OutlineFeature::updating_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Updates),
			RenderAccess::Write(Styles)
		};

		static void Execute(OutlineFeature& feature, RenderPass& pass);
	};

	struct OutlineFeature::UpdateStage::MaskStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Lighting;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::All;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Outline);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Mask),
			RenderAccess::Write(Widest)
		};

		static void Execute(OutlineFeature& feature, RenderPass& pass);
	};

	struct OutlineFeature::ResolveStage
	{
		struct SceneStep;

		struct CanvasStep;

		using Sequence = RenderSequence<SceneStep, CanvasStep>;
	};

	struct OutlineFeature::ResolveStage::SceneStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Highlight;

		SC_CONSTEX Bool OutlineFeature::* enableCondition_ = &OutlineFeature::outlining_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Outline);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Mask),
			RenderAccess::Read(Styles),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Write(Source),
			RenderAccess::Write(Widest)
		};

		static void Execute(OutlineFeature& feature, RenderPass& pass);
	};

	struct OutlineFeature::ResolveStage::CanvasStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Highlight;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Canvas;

		SC_CONSTEX Bool OutlineFeature::* enableCondition_ = &OutlineFeature::outlining_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Outline);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Mask),
			RenderAccess::Read(Styles),
			RenderAccess::Write(Source),
			RenderAccess::Write(Widest)
		};

		static void Execute(OutlineFeature& feature, RenderPass& pass);
	};

	struct OutlineFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Highlight;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::All;

		SC_CONSTEX Bool OutlineFeature::* enableCondition_ = &OutlineFeature::outlining_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Outline);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Source),
			RenderAccess::Read(Widest),
			RenderAccess::Target(HUDFeature::Display)
		};

		static void Execute(OutlineFeature& feature, RenderPass& pass);
	};
}
