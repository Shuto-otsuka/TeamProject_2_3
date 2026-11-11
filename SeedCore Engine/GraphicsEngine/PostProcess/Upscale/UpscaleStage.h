#pragma once
#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct UpscaleFeature::TaauResolveStage :RenderStage
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

	struct UpscaleFeature::PrepareGuidesStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool UpscaleFeature::* enableCondition_ = &UpscaleFeature::dlssRayReconstruction_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(MaterialFeature::SurfaceBaseColor),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Write(NormalRoughness),
			RenderAccess::Write(SpecularAlbedo),
			RenderAccess::Write(DiffuseAlbedo)
		};

		static void Execute(UpscaleFeature& feature, RenderPass& pass);
	};

	struct UpscaleFeature::DlssRayReconstructionStage :RenderStage
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

	struct UpscaleFeature::DlssSuperResolutionStage :RenderStage
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
