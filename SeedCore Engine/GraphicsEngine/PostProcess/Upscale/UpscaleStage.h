#pragma once
#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct UpscaleFeature::BuildStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool UpscaleFeature::* enableCondition_ = &UpscaleFeature::dlssRayReconstruction_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(MaterialFeature::SurfaceBaseColor),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Write(NormalRoughness),
			RenderAccess::Write(SpecularAlbedo),
			RenderAccess::Write(DiffuseAlbedo)
		};

		static void Execute(UpscaleFeature& feature, RenderPass& pass);
	};

	struct UpscaleFeature::ResolveStage
	{
		struct TaauStep;

		struct RayReconstructionStep;

		struct SuperResolutionStep;

		using Sequence = RenderSequence<TaauStep, RayReconstructionStep, SuperResolutionStep>;
	};

	struct UpscaleFeature::ResolveStage::TaauStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool UpscaleFeature::* enableCondition_ = &UpscaleFeature::taau_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(RadianceFeature::Color),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::History(History),
			RenderAccess::Write(History),
			RenderAccess::Write(Color)
		};

		static void Execute(UpscaleFeature& feature, RenderPass& pass);
	};

	struct UpscaleFeature::ResolveStage::RayReconstructionStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Callback;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool UpscaleFeature::* enableCondition_ = &UpscaleFeature::dlssRayReconstruction_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(RadianceFeature::Color),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Read(NormalRoughness),
			RenderAccess::Read(SpecularAlbedo),
			RenderAccess::Read(DiffuseAlbedo),
			RenderAccess::Write(Color)
		};

		static void Execute(UpscaleFeature& feature, RenderPass& pass);
	};

	struct UpscaleFeature::ResolveStage::SuperResolutionStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Callback;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool UpscaleFeature::* enableCondition_ = &UpscaleFeature::dlssSuperResolution_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(RadianceFeature::Color),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Write(Color)
		};

		static void Execute(UpscaleFeature& feature, RenderPass& pass);
	};
}
