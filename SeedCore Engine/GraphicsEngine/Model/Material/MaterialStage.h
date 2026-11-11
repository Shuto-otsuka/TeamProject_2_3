#pragma once
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct MaterialFeature::ClassifyStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Write(Buckets),
			RenderAccess::Write(Velocity),
			RenderAccess::Write(SurfaceRadiance)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};

	struct MaterialFeature::PrefixSumStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Buckets)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};

	struct MaterialFeature::ScatterStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Write(Buckets),
			RenderAccess::Write(SortedPixels)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};

	struct MaterialFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(SortedPixels),
			RenderAccess::Read(Buckets),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::Write(SurfaceBaseColor),
			RenderAccess::Write(SurfaceNormalRoughness),
			RenderAccess::Write(SurfaceTangent),
			RenderAccess::Write(Velocity),
			RenderAccess::Write(SurfaceRadiance)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};
}
