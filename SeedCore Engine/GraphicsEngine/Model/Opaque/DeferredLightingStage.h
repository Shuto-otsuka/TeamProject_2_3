#pragma once
#include <GraphicsEngine/Model/Opaque/DeferredLightingFeature.h>
#include <GraphicsEngine/Illumination/DirectLight/DirectLightFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>

namespace SeedCore
{
	struct DeferredLightingFeature::DrawStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Lighting;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::SurfaceBaseColor),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(MaterialFeature::SurfaceTangent),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Read(GeometryFeature::VisibilityTexcoord),
			RenderAccess::Read(ModelFeature::Instances),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Read(DirectLightFeature::PunctualRadiance),
			RenderAccess::Read(SkyFeature::DiffuseIrradiance),
			RenderAccess::Read(SkyFeature::SpecularPrefiltered),
			RenderAccess::Read(SkyFeature::BrdfLut),
			RenderAccess::Target(MaterialFeature::SurfaceRadiance)
		};

		static void Execute(DeferredLightingFeature& feature, RenderPass& pass);
	};
}
