#pragma once
#include <GraphicsEngine/Culling/CullingFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct CullingFeature::CullingStage
	{
		struct EarlyStep;

		struct LateStep;

		struct TransparentStep;

		using Sequence = RenderSequence<EarlyStep, LateStep, TransparentStep>;
	};

	struct CullingFeature::CullingStage::EarlyStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Counts),
			RenderAccess::History(GeometryFeature::HierarchicalDepth),
			RenderAccess::Write(Rejected),
			RenderAccess::Write(DrawLists),
			RenderAccess::Write(DrawArguments),
			RenderAccess::Write(TransparentArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};

	struct CullingFeature::CullingStage::LateStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Counts),
			RenderAccess::Read(GeometryFeature::HierarchicalDepth),
			RenderAccess::Read(Rejected),
			RenderAccess::Write(DrawLists),
			RenderAccess::Write(DrawArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};

	struct CullingFeature::CullingStage::TransparentStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Transparent;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Transparent);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Counts),
			RenderAccess::Read(GeometryFeature::HierarchicalDepth),
			RenderAccess::Write(TransparentLists),
			RenderAccess::Write(TransparentArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};
}
