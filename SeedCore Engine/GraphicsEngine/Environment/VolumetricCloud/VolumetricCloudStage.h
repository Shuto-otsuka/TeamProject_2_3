#pragma once
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>

namespace SeedCore
{
	struct VolumetricCloudFeature::BuildStage
	{
		struct ShapeStep;

		struct DetailStep;

		using Sequence = RenderSequence<ShapeStep, DetailStep>;
	};

	struct VolumetricCloudFeature::BuildStage::ShapeStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool VolumetricCloudFeature::* enableCondition_ = &VolumetricCloudFeature::baking_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Shape)
		};

		static void Execute(VolumetricCloudFeature& feature, RenderPass& pass);
	};

	struct VolumetricCloudFeature::BuildStage::DetailStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool VolumetricCloudFeature::* enableCondition_ = &VolumetricCloudFeature::baking_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Detail)
		};

		static void Execute(VolumetricCloudFeature& feature, RenderPass& pass);
	};

	struct VolumetricCloudFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool VolumetricCloudFeature::* enableCondition_ = &VolumetricCloudFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::VolumetricCloud);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Shape),
			RenderAccess::Read(Detail),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Write(Radiance)
		};

		static void Execute(VolumetricCloudFeature& feature, RenderPass& pass);
	};
}
