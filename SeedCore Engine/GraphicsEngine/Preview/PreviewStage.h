#pragma once
#include <GraphicsEngine/Preview/PreviewFeature.h>

namespace SeedCore
{
	struct PreviewFeature::DrawStage
	{
		struct MeshStep;

		struct LineStep;

		using Sequence = RenderSequence<MeshStep, LineStep>;
	};

	struct PreviewFeature::DrawStage::MeshStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Instances),
			RenderAccess::Read(Meshlets),
			RenderAccess::Read(Bones),
			RenderAccess::DepthWrite(Depth),
			RenderAccess::Target(Output)
		};

		static void Execute(PreviewFeature& feature, RenderPass& pass);
	};

	struct PreviewFeature::DrawStage::LineStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX Bool PreviewFeature::* enableCondition_ = &PreviewFeature::lining_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Lines),
			RenderAccess::Target(Output)
		};

		static void Execute(PreviewFeature& feature, RenderPass& pass);
	};
}
