#pragma once
#include <GraphicsEngine/Culling/CullingFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct CullingFeature::CameraEarlyStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Read(ModelFeature::Counts),
			RenderAccess::History(GeometryFeature::HierarchicalDepth),
			RenderAccess::Write(Rejected),
			RenderAccess::Write(DrawLists),
			RenderAccess::Write(DrawArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};

	struct CullingFeature::CameraLateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Read(ModelFeature::Counts),
			RenderAccess::Read(GeometryFeature::HierarchicalDepth),
			RenderAccess::Read(Rejected),
			RenderAccess::Write(DrawLists),
			RenderAccess::Write(DrawArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};
}
