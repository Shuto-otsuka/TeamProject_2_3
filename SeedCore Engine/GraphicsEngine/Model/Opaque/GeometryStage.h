#pragma once
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Culling/CullingFeature.h>

namespace SeedCore
{
	struct GeometryFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Counts)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::DrawStage
	{
		struct DepthStep;

		struct PyramidStep;

		struct VisibilityStep;

		using Sequence = RenderSequence<DepthStep, PyramidStep, VisibilityStep>;
	};

	struct GeometryFeature::DrawStage::DepthStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX Uint32 repeat_ = 2;

		SC_CONSTEX RenderQueue queue_[] = { RenderQueue::Geometry, RenderQueue::Occlusion };

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::Write(Debug),
			RenderAccess::DepthWrite(Depth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::DrawStage::PyramidStep :RenderStage
	{
		SC_CONSTEX Uint32 repeat_ = 2;

		SC_CONSTEX RenderQueue queue_[] = { RenderQueue::Geometry, RenderQueue::Illumination };

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Depth),
			RenderAccess::Write(HierarchicalDepth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};

	struct GeometryFeature::DrawStage::VisibilityStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::Write(Debug),
			RenderAccess::Target(VisibilityId),
			RenderAccess::Target(VisibilityTexcoord),
			RenderAccess::DepthTest(Depth)
		};

		static void Execute(GeometryFeature& feature, RenderPass& pass);
	};
}
