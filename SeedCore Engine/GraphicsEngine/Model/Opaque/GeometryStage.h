#pragma once
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Culling/CullingFeature.h>

namespace SeedCore
{
	struct GeometryFeature::DepthEarlyStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::DepthWrite(Depth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::EarlyDepthResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Depth),
			RenderAccess::Write(HierarchicalDepth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::DepthLateStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::DepthWrite(Depth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::DepthResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Depth),
			RenderAccess::Write(HierarchicalDepth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::VisibilityStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::Target(VisibilityId),
			RenderAccess::Target(VisibilityTexcoord),
			RenderAccess::DepthTest(Depth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};
}
