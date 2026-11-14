#pragma once
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct MaterialFeature::BuildStage
	{
		struct ClassifyStep;

		struct PrefixSumStep;

		struct ScatterStep;

		using Sequence = RenderSequence<ClassifyStep, PrefixSumStep, ScatterStep>;
	};

	struct MaterialFeature::BuildStage::ClassifyStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Write(Buckets),
			RenderAccess::Write(Velocity),
			RenderAccess::Write(Radiance)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};

	struct MaterialFeature::BuildStage::PrefixSumStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Buckets)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};

	struct MaterialFeature::BuildStage::ScatterStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Write(Buckets),
			RenderAccess::Write(Pixels)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};

	struct MaterialFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Pixels),
			RenderAccess::Read(Buckets),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::Morphs),
			RenderAccess::Write(BaseColor),
			RenderAccess::Write(NormalRoughness),
			RenderAccess::Write(Tangent),
			RenderAccess::Write(Velocity),
			RenderAccess::Write(Radiance),
			RenderAccess::Write(Mip)
		};

		static void Execute(MaterialFeature& feature, RenderPass& pass);
	};
}
